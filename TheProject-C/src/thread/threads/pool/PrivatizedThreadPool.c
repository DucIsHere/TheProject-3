#include "PrivatizedThreadPool.h"
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <processthreadsapi.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

static DynamicTaskArray* alloc_task_array(size_t capacity) {
    DynamicTaskArray* arr = (DynamicTaskArray*)malloc(sizeof(DynamicTaskArray) + sizeof(_Atomic(TaskItem)) * capacity);
    if (!arr) return NULL;
    arr->capacity = capacity;
    arr->mask = capacity - 1;
    return arr;
}

static void chase_level_init(ChaseLevDeque* deque, size_t initial_capacity) {
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
            if (!atomic_compare_exchange_strong_explicit(&deque->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
                atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
                return false;
            }
            atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        }
        return true;
    } else {
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
// IO_URING HELPER
// =============================================================================

static void init_io_uring(IoUringEngine* io) {
#if defined(__linux__) && defined(__NR_io_uring_setup)
    struct io_uring_params {
        uint32_t sq_entries;
        uint32_t cq_entries;
        uint32_t flags;
        uint32_t sq_thread_cpu;
        uint32_t sq_thread_idle;
        uint32_t features;
        uint32_t wq_fd;
        uint32_t resv[3];
        struct {
            uint32_t rd;
            uint32_t wr;
            uint32_t flags;
            uint32_t array;
        } sq_off;
        struct {
            uint32_t rd;
            uint32_t wr;
            uint32_t flags;
            uint32_t overflow;
            uint32_t user_addr;
        } cq_off;
    } p;
    memset(&p, 0, sizeof(p));
    long ret = syscall(__NR_io_uring_setup, 32, &p);
    if (ret >= 0) {
        io->ring_fd = (int)ret;
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
// WORKER ROUTINE & NATIVE THREAD DISPATCHER
// =============================================================================

typedef struct WorkerInitArgs {
    PrivatizedThreadPool* pool;
    size_t worker_id;
} WorkerInitArgs;

static void execute_task_item(TaskItem* item) {
    if (item->func) {
        void* result = item->func(NULL, item->user_data);
        if (item->future_result) {
            atomic_store_explicit(item->future_result, result, memory_order_release);
        }
        if (item->counter) {
            atomic_fetch_sub_explicit(item->counter, 1, memory_order_release);
        }
    }
}

#if defined(_WIN32) || defined(_WIN64)
static DWORD WINAPI privatized_worker_routine(LPVOID lpParam)
#else
static void* privatized_worker_routine(void* lpParam)
#endif
{
    WorkerInitArgs* args = (WorkerInitArgs*)lpParam;
    PrivatizedThreadPool* pool = args->pool;
    size_t id = args->worker_id;
    free(args);

    Worker* self = &pool->workers[id];

    while (!atomic_load_explicit(&pool->stop, memory_order_relaxed)) {
        TaskItem item;
        bool found = false;

        // 1. Lấy Task từ Local Cache Slot hoặc Chase-Lev Deque
        if (self->has_cache) {
            item = self->cache_task;
            self->has_cache = false;
            found = true;
        } else {
            found = chase_lev_pop(self->dynamic_deque, &item);
        }

        // 2. Work Stealing từ các Worker khác
        if (!found) {
            for (size_t i = 0; i < pool->num_workers; ++i) {
                size_t victim = (id + i + 1) % pool->num_workers;
                if (chase_lev_steal(pool->workers[victim].dynamic_deque, &item)) {
                    found = true;
                    break;
                }
            }
        }

        // 3. Thực thi Task hoặc Pause CPU
        if (found) {
            execute_task_item(&item);
            atomic_fetch_add_explicit(&pool->total_tasks, 1, memory_order_relaxed);
        } else {
            _mm_pause();
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    return 0;
#else
    return NULL;
#endif
}

// =============================================================================
// PUBLIC APIS IMPLEMENTATION
// =============================================================================

PrivatizedThreadPool* privatized_pool_create(size_t num_workers) {
    if (num_workers == 0) num_workers = 1;

    PrivatizedThreadPool* pool = (PrivatizedThreadPool*)calloc(1, sizeof(PrivatizedThreadPool));
    if (!pool) return NULL;

    pool->num_workers = num_workers;
    pool->base_pool = pool_create(num_workers); // Kế thừa/Tạo Base Engine từ thread_pool.h
    pool->workers = (Worker*)calloc(num_workers, sizeof(Worker));

#if defined(_WIN32) || defined(_WIN64)
    pool->threads = (HANDLE*)malloc(sizeof(HANDLE) * num_workers);
#else
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_workers);
#endif

    for (size_t i = 0; i < num_workers; ++i) {
        Worker* w = &pool->workers[i];
#if defined(_WIN32) || defined(_WIN64)
        InitializeCriticalSection(&w->lock);
        InitializeConditionVariable(&w->cv);
#else
        pthread_mutex_init(&w->lock, NULL);
        pthread_cond_init(&w->cv, NULL);
#endif

        w->dynamic_deque = (ChaseLevDeque*)malloc(sizeof(ChaseLevDeque));
        chase_lev_init(w->dynamic_deque, PRIV_QUEUE_CAPACITY);
        init_io_uring(&w->io_uring);

        WorkerInitArgs* init_args = (WorkerInitArgs*)malloc(sizeof(WorkerInitArgs));
        init_args->pool = pool;
        init_args->worker_id = i;

#if defined(_WIN32) || defined(_WIN64)
        pool->threads[i] = CreateThread(NULL, 0, privatized_worker_routine, init_args, 0, NULL);
#else
        pthread_create(&pool->threads[i], NULL, privatized_worker_routine, init_args);
#endif
    }

    return pool;
}

void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core) {
    privatized_pool_submit_ex(pool, func, arg, prio, core, NULL, NULL);
}

void privatized_pool_submit_ex(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core, _Atomic(size_t)* counter, _Atomic(void*)* future_out) {
    TaskItem item = {
        .func = func,
        .user_data = arg,
        .priority = prio,
        .target_core = core,
        .counter = counter,
        .future_result = future_out
    };

    if (counter) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }

    // Đẩy vào Local Worker 0 (hoặc vòng tròn qua các Workers)
    Worker* local_worker = &pool->workers[0];
    chase_lev_push(local_worker->dynamic_deque, item);
}

void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count) {
    if (!pool || !items || count == 0) return;
    Worker* local_worker = &pool->workers[0];
    for (size_t i = 0; i < count; ++i) {
        chase_lev_push(local_worker->dynamic_deque, items[i]);
    }
}

size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch) {
    if (!worker || !out_batch || max_batch == 0) return 0;
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
    if (!worker || !worker->io_uring.enabled) return false;
    (void)fd; (void)buf; (void)bytes; (void)offset; (void)on_complete; (void)user_data;
    // Chờ gắn SQE / CQE cho io_uring Linux
    return true;
}

void privatized_pool_destroy(PrivatizedThreadPool* pool) {
    if (!pool) return;

    atomic_store_explicit(&pool->stop, true, memory_order_release);

    for (size_t i = 0; i < pool->num_workers; ++i) {
#if defined(_WIN32) || defined(_WIN64)
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
        DeleteCriticalSection(&pool->workers[i].lock);
#else
        pthread_join(pool->threads[i], NULL);
        pthread_mutex_destroy(&pool->workers[i].lock);
        pthread_cond_destroy(&pool->workers[i].cv);
#endif
        DynamicTaskArray* arr = atomic_load_explicit(&pool->workers[i].dynamic_deque->array, memory_order_relaxed);
        free(arr);
        free(pool->workers[i].dynamic_deque);
    }

    if (pool->base_pool) {
        pool_destroy(pool->base_pool);
    }

    free(pool->threads);
    free(pool->workers);
    free(pool);
}
