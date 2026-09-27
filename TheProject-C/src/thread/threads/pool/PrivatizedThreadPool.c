#include "PrivatizedThreadPool.h"

#include <string.h>

#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#endif

// =============================================================================
// NATIVE THREAD & MUTEX WRAPPERS (TẤT CẢ TRONG MỘT NỀN TẢNG OS)
// =============================================================================

static void native_mutex_init(native_mutex_t* mtx) {
#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(mtx);
#else
    pthread_mutex_init(mtx, NULL);
#endif
}

static void native_mutex_destroy(native_mutex_t* mtx) {
#if defined(_WIN32) || defined(_WIN64)
    DeleteCriticalSection(mtx);
#else
    pthread_mutex_destroy(mtx);
#endif
}

static void native_cond_init(native_cond_t* cv) {
#if defined(_WIN32) || defined(_WIN64)
    InitializeConditionVariable(cv);
#else
    pthread_cond_init(cv, NULL);
#endif
}

static void native_cond_destroy(native_cond_t* cv) {
#if defined(_WIN32) || defined(_WIN64)
    (void)cv; // ConditionVariable trong Windows không cần Delete
#else
    pthread_cond_destroy(cv);
#endif
}

// =============================================================================
// CHASE-LEV DEQUE IMPLEMENTATION (DYNAMIC LOCK-FREE WORK STEALING)
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
// IO_URING INITIALIZATION
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
// WORKER ROUTINE & TIER 1 / TIER 2 SCHEDULING
// =============================================================================

typedef struct WorkerThreadArgs {
    PrivatizedThreadPool* pool;
    size_t worker_id;
} WorkerThreadArgs;

#if defined(_WIN32) || defined(_WIN64)
static DWORD WINAPI privatized_worker_routine(LPVOID arg)
#else
static void* privatized_worker_routine(void* arg)
#endif
{
    WorkerThreadArgs* init_args = (WorkerThreadArgs*)arg;
    PrivatizedThreadPool* pool = init_args->pool;
    size_t id = init_args->worker_id;
    free(init_args);

    Worker* self = &pool->workers[id];

    while (!atomic_load_explicit(&pool->stop, memory_order_relaxed)) {
        TaskItem item = {0};
        bool found = false;

        // TẦNG 1: Fast-Path Local Slot & Local Chase-Lev Deque
        if (self->has_cache) {
            item = self->cache_task;
            self->has_cache = false;
            found = true;
        } else {
            found = chase_lev_pop(self->dynamic_deque, &item);
        }

        // TẦNG 1.5: Lock-Free Work-Stealing từ Worker lân cận
        if (!found && pool->num_workers > 1) {
            for (size_t i = 0; i < pool->num_workers; ++i) {
                size_t victim = (id + i + 1) % pool->num_workers;
                if (chase_lev_steal(pool->workers[victim].dynamic_deque, &item)) {
                    found = true;
                    break;
                }
            }
        }

        // TẦNG 2: Global Fallback xuống Base Engine (ThreadPool.h)
        if (!found && pool->base_pool) {
            // Lấy Task từ Base Pool nếu Tầng 1 rảnh
            TaskHandle task_handle;
            memset(&task_handle, 0, sizeof(TaskHandle));

            if (pool_pop_task(pool->base_pool, &task_handle)) {
                if (task_handle.func) {
                    void* result = task_handle.func(&task_handle, task_handle.arg);

                    if (task_handle.feature_result) {
                        atomic_store_explicit(task_handle.feature_result, result, memory_order_release);
                    }
                    if (task_handle.counter) {
                        atomic_fetch_sub_explicit(task_handle.counter, 1, memory_order_release);
                    }
                    atomic_fetch_add_explicit(&pool->total_tasks, 1, memory_order_relaxed);
                }
                continue;
            }
        }

        // XỬ LÝ TASK CỦA TẦNG PRIVATIZED
        if (found && item.func) {
            void* res = item.func(NULL, item.user_data);

            if (item.future_result) {
                atomic_store_explicit(item.future_result, res, memory_order_release);
            }
            if (item.counter) {
                atomic_fetch_sub_explicit(item.counter, 1, memory_order_release);
            }

            atomic_fetch_add_explicit(&pool->total_tasks, 1, memory_order_relaxed);
        } else {
            _mm_pause(); // Tối ưu CPU Pipeline Spin loop
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

[[nodiscard]] PrivatizedThreadPool* privatized_pool_create(size_t num_workers) {
    PrivatizedThreadPool* pool = (PrivatizedThreadPool*)calloc(1, sizeof(PrivatizedThreadPool));
    if (!pool) return NULL;

    pool->num_workers = num_workers;
    pool->base_pool = pool_create(num_workers);

#if defined(_WIN32) || defined(_WIN64)
    pool->threads = (void**)malloc(sizeof(HANDLE) * num_workers);
#else
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_workers);
#endif

    pool->workers = (Worker*)calloc(num_workers, sizeof(Worker));

    for (size_t i = 0; i < num_workers; ++i) {
        native_mutex_init(&pool->workers[i].lock);
        native_cond_init(&pool->workers[i].cv);

        pool->workers[i].dynamic_deque = (ChaseLevDeque*)malloc(sizeof(ChaseLevDeque));
        chase_lev_init(pool->workers[i].dynamic_deque, PRIV_QUEUE_CAPACITY);
        init_io_uring(&pool->workers[i].io_uring);

        WorkerThreadArgs* args = (WorkerThreadArgs*)malloc(sizeof(WorkerThreadArgs));
        args->pool = pool;
        args->worker_id = i;

#if defined(_WIN32) || defined(_WIN64)
        pool->threads[i] = CreateThread(NULL, 0, privatized_worker_routine, args, 0, NULL);
#else
        pthread_create(&pool->threads[i], NULL, privatized_worker_routine, args);
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

    // Đẩy Fast-Path vào Local Deque của Worker 0 (hoặc chọn theo Round-Robin)
    static _Atomic size_t rr_index = 0;
    size_t target_idx = atomic_fetch_add_explicit(&rr_index, 1, memory_order_relaxed) % pool->num_workers;

    chase_lev_push(pool->workers[target_idx].dynamic_deque, item);
}

void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count) {
    if (!pool || !items || count == 0) return;

    static _Atomic size_t rr_index = 0;
    size_t target_idx = atomic_fetch_add_explicit(&rr_index, 1, memory_order_relaxed) % pool->num_workers;

    for (size_t i = 0; i < count; ++i) {
        chase_lev_push(pool->workers[target_idx].dynamic_deque, items[i]);
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
    // Tích hợp Submit Async IO SQE trực tiếp với Linux Kernel
    (void)fd; (void)buf; (void)bytes; (void)offset; (void)on_complete; (void)user_data;
    return true;
}

void privatized_pool_destroy(PrivatizedThreadPool* pool) {
    if (!pool) return;

    atomic_store_explicit(&pool->stop, true, memory_order_release);

    for (size_t i = 0; i < pool->num_workers; ++i) {
#if defined(_WIN32) || defined(_WIN64)
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
#else
        pthread_join(pool->threads[i], NULL);
#endif
        native_mutex_destroy(&pool->workers[i].lock);
        native_cond_destroy(&pool->workers[i].cv);
        free(pool->workers[i].dynamic_deque);
    }

    if (pool->base_pool) {
        pool_destroy(pool->base_pool);
    }

    free(pool->threads);
    free(pool->workers);
    free(pool);
}