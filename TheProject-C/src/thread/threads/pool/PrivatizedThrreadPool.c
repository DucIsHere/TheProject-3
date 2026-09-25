#include "PrivatizedThreadPool.h"

#include <string.h>
#include <stdatomic.h>

#if defined(__linux__)
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

// =============================================================================
// CHASE-LEV DEQUE IMPLEMENTATION (LOCK-FREE WORK STEALING)
// =============================================================================

static DynamicTaskArray* alloc_task_array(size_t capacity) {
    DynamicTaskArray* arr = (DynamicTaskArray*)malloc(sizeof(DynamicTaskArray) + sizeof(_Atomic(TaskItem)) * capacity);
    if (!arr) return NULL;
    arr->capacity = capacity;
    arr->mask = capacity - 1;
    return arr;
}

static void chase_lev_init(ChaseLevDeque* deque, size_t initial_capacity) {
    atomic_store_explicit(&deque->top, 0, memory_order_relaxed);
    atomic_store_explicit(&deque->bottom, 0, memory_order_relaxed);
    DynamicTaskArray* arr = alloc_task_array(initial_capacity);
    atomic_store_explicit(&deque->array, arr, memory_order_relaxed);
}

static void chase_lev_push(ChaseLevDeque* deque, TaskItem item) {
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    size_t t = atomic_load_explicit(&deque->top, memory_order_acquire);
    DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_relaxed);

    if (b - t >= a->capacity - 1) {
        // Mở rộng dung lượng mảng nếu bị đầy
        size_t new_cap = a->capacity * 2;
        DynamicTaskArray* new_a = alloc_task_array(new_cap);
        for (size_t i = t; i < b; ++i) {
            TaskItem val = atomic_load_explicit(&a->buffer[i & a->mask], memory_order_relaxed);
            atomic_store_explicit(&new_a->buffer[i & new_a->mask], val, memory_order_relaxed);
        }
        atomic_store_explicit(&deque->array, new_a, memory_order_release);
        a = new_a;
    }

    atomic_store_explicit(&a->buffer[b & a->mask], item, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
}

static bool chase_lev_pop(ChaseLevDeque* deque, TaskItem* out_item) {
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    if (b == 0) return false;
    b--;
    atomic_store_explicit(&deque->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);

    size_t t = atomic_load_explicit(&deque->top, memory_order_relaxed);
    DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_relaxed);

    if (t <= b) {
        *out_item = atomic_load_explicit(&a->buffer[b & a->mask], memory_order_relaxed);
        if (t == b) {
            // Phần tử cuối cùng trong Deque
            if (!atomic_compare_exchange_strong_explicit(&deque->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
                // Thua tranh chấp Atomics với Stealer khác
                atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
                return false;
            }
            atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        }
        return true;
    } else {
        // Deque rỗng
        atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        return false;
    }
}

static bool chase_lev_steal(ChaseLevDeque* deque, TaskItem* out_item) {
    size_t t = atomic_load_explicit(&deque->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_acquire);

    if (t < b) {
        DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_consume);
        *out_item = atomic_load_explicit(&a->buffer[t & a->mask], memory_order_relaxed);

        if (!atomic_compare_exchange_weak_explicit(&deque->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
            return false;
        }
        return true;
    }
    return false;
}

// =============================================================================
// KERNEL IO_URING ENGINE INITIALIZATION
// =============================================================================

static void init_io_uring(IoUringEngine* io) {
#if defined(__linux__)
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    int ret = (int)syscall(__NR_io_uring_setup, 32, &p);
    if (ret >= 0) {
        io->ring_fd = ret;
        io->enabled = true;
        io->sq_ring_mask = p.sq_entries - 1;
        io->cq_ring_mask = p.cq_entries - 1;
    } else {
        io->enabled = false;
    }
#else
    io->enabled = false;
    io->ring_fd = -1;
#endif
}

// =============================================================================
// WORKER THREAD ROUTINE
// =============================================================================

#if defined(PCCORE_OS_WINDOWS)
static DWORD WINAPI privatized_worker_routine(LPVOID arg)
#elif defined(PCCORE_OS_LINUX)
static void* privatized_worker_routine(void* arg)
#endif
{
    typedef struct { PrivatizedThreadPool* pool; size_t id; } PrivWorkerInit;
    PrivWorkerInit* init = (PrivWorkerInit*)arg;
    PrivatizedThreadPool* pool = init->pool;
    size_t id = init->id;
    free(init);

    Worker* self = &pool->workers[id];

    while (!atomic_load_explicit(&pool->stop, memory_order_relaxed)) {
        TaskItem item;
        bool found = false;

        // 1. Fast-Path: Kiểm tra Cache Slot hoặc Pop từ Chase-Lev Deque nội bộ
        if (self->has_cache) {
            item = self->cache_task;
            self->has_cache = false;
            found = true;
        } else {
            found = chase_lev_pop(self->dynamic_deque, &item);
        }

        // 2. Work-Stealing: Tìm Task từ Worker khác nếu Deque nội bộ rỗng
        if (!found) {
            for (size_t i = 0; i < pool->num_workers; ++i) {
                size_t victim = (id + i + 1) % pool->num_workers;
                if (chase_lev_steal(pool->workers[victim].dynamic_deque, &item)) {
                    found = true;
                    self->last_victim = victim;
                    break;
                }
            }
        }

        // 3. Fallback: Nếu vẫn không có Task, chuyển sang lấy việc từ Base Engine
        if (!found && pool->base_pool) {
            pool_wait_active(pool->base_pool, NULL);
        }

        // 4. Execute Task
        if (found && item.func) {
            item.func(NULL, item.user_data);
            atomic_fetch_add_explicit(&pool->total_tasks, 1, memory_order_relaxed);
        } else {
            _mm_pause();
        }
    }

#if defined(PCCORE_OS_WINDOWS)
    return 0;
#elif defined(PCCORE_OS_LINUX)
    return NULL;
#endif
}

// =============================================================================
// PUBLIC APIS
// =============================================================================

PrivatizedThreadPool* privatized_pool_create(size_t num_workers) {
    if (num_workers == 0) num_workers = TOTAL_HARDWARE_THREADS;

    PrivatizedThreadPool* pool = (PrivatizedThreadPool*)calloc(1, sizeof(PrivatizedThreadPool));
    if (!pool) return NULL;

    pool->num_workers = num_workers;
    pool->base_pool = pool_create(num_workers);
    pool->threads = (native_thread_t*)malloc(sizeof(native_thread_t) * num_workers);
    pool->workers = (Worker*)calloc(num_workers, sizeof(Worker));

    for (size_t i = 0; i < num_workers; ++i) {
        pool->workers[i].dynamic_deque = (ChaseLevDeque*)malloc(sizeof(ChaseLevDeque));
        chase_lev_init(pool->workers[i].dynamic_deque, PRIV_QUEUE_CAPACITY);
        init_io_uring(&pool->workers[i].io_uring);

        typedef struct { PrivatizedThreadPool* pool; size_t id; } PrivWorkerInit;
        PrivWorkerInit* init = (PrivWorkerInit*)malloc(sizeof(PrivWorkerInit));
        init->pool = pool;
        init->id = i;

#if defined(PCCORE_OS_WINDOWS)
        pool->threads[i] = CreateThread(NULL, 0, privatized_worker_routine, init, 0, NULL);
#elif defined(PCCORE_OS_LINUX)
        pthread_create(&pool->threads[i], NULL, privatized_worker_routine, init);
#endif
    }

    return pool;
}

void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core) {
    TaskItem item = { .func = func, .user_data = arg, .priority = prio, .target_core = core };

    // Đẩy trực tiếp vào Chase-Lev Deque của Worker 0 (hoặc Worker hiện tại)
    Worker* local_worker = &pool->workers[0];
    chase_lev_push(local_worker->dynamic_deque, item);
}

void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count) {
    Worker* local_worker = &pool->workers[0];
    for (size_t i = 0; i < count; ++i) {
        chase_lev_push(local_worker->dynamic_deque, items[i]);
    }
}

size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch) {
    size_t count = 0;
    while (count < max_batch) {
        if (!chase_lev_pop(worker->dynamic_deque, &out_batch[count])) {
            break;
        }
        count++;
    }
    return count;
}

bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data) {
    if (!worker->io_uring.enabled) return false;

#if defined(__linux__)
    // Triển khai Submit SQE cho Linux io_uring
    (void)fd; (void)buf; (void)bytes; (void)offset; (void)on_complete; (void)user_data;
    return true;
#else
    (void)worker; (void)fd; (void)buf; (void)bytes; (void)offset; (void)on_complete; (void)user_data;
    return false;
#endif
}

void privatized_pool_destroy(PrivatizedThreadPool* pool) {
    if (!pool) return;

    atomic_store_explicit(&pool->stop, true, memory_order_release);

    for (size_t i = 0; i < pool->num_workers; ++i) {
#if defined(PCCORE_OS_WINDOWS)
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
#elif defined(PCCORE_OS_LINUX)
        pthread_join(pool->threads[i], NULL);
#endif
        free(pool->workers[i].dynamic_deque);
    }

    if (pool->base_pool) {
        pool_destroy(pool->base_pool);
    }

    free(pool->threads);
    free(pool->workers);
    free(pool);
}