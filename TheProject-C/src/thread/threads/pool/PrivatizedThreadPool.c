#pragma GCC optimize("O3", "unroll-loops", "tree-vectorize")
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

#define _GNU_SOURCE

#include "PrivatizedThreadPool.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/io_uring.h>
#include <immintrin.h> // Tối ưu x86_64 (_mm_pause, AVX2/AVX-512)

static thread_local Worker* g_local_worker = nullptr;
static thread_local size_t g_worker_id = 0;

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS GIỮ NGUYÊN TỪ CŨ
// ----------------------------------------------------------------------------

[[gnu::always_inline]] static inline uint32_t fast_modulo(uint32_t x, uint32_t N) {
    return (uint32_t)(((uint64_t)x * (uint64_t)N) >> 32);
}

[[gnu::always_inline]] static inline uint32_t pcg_rand(uint64_t* state) {
    uint64_t current = *state;
    *state = current * 6364136223846793005ULL + 0xda3e39cb94b95bdbULL;
    return (uint32_t)((current ^ (current >> 22)) >> (22 + (current >> 61)));
}

static void priv_queue_init(PrivatizedTaskQueue* q, CoreType core) {
    q->buffer_mask = PRIv_QUEUE_CAPACITY - 1;
    q->bound_core = core;
    atomic_store_explicit(&q->front, 0, memory_order_relaxed);
    atomic_store_explicit(&q->back, 0, memory_order_relaxed);
    atomic_store_explicit(&q->num_items, 0, memory_order_relaxed);

#pragma GCC ivdep
#pragma clang loop vectorize(enable)
    for (size_t i = 0; i < PRIV_QUEUE_CAPACITY; i++) {
        atomic_store_explicit(&q->buffer[i].sequence, i, memory_order_relaxed);
    }
}

[[gnu::always_inline]] static inline bool priv_queue_enqueue(PrivatizedTaskQueue* q, TaskItem data) {
    size_t pos = atomic_load_explicit(&q->front, memory_order_relaxed);
    while (1) {
        QueueCell* cell = &q->buffer[pos & q->buffer_mask];
        size_t seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)pos;

        if (dif == 0) [[likely]] {
            if (atomic_compare_exchange_weak_explicit(&q->front, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed)) {
                cell->data = data;
                atomic_fetch_add_explicit(&q->num_items, 1, memory_order_relaxed);
                atomic_store_explicit(&cell->sequence, pos + 1, memory_order_release);
                return true;
            }
        } else if (dif < 0) [[unlikely]] {
            return false;
        } else {
            pos = atomic_load_explicit(&q->front, memory_order_relaxed);
        }
    }
}

[[gnu::always_inline]] static inline bool priv_queue_dequeue(PrivatizedTaskQueue* q, TaskItem* out_data) {
    size_t pos = atomic_load_explicit(&q->back, memory_order_relaxed);
    while (1) {
        QueueCell* cell = &q->buffer[pos & q->buffer_mask];
        size_t seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

        if (dif == 0) [[likely]] {
            if (atomic_compare_exchange_weak_explicit(&q->back, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed)) {
                *out_data = cell->data;
                atomic_fetch_sub_explicit(&q->num_items, 1, memory_order_relaxed);
                atomic_store_explicit(&cell->sequence, pos + q->buffer_mask + 1, memory_order_release);
                return true;
            }
        } else if (dif < 0) [[unlikely]] {
            return false;
        } else {
            pos = atomic_load_explicit(&q->back, memory_order_relaxed);
        }
    }
}

// ----------------------------------------------------------------------------
// NÂNG CẤP 1 & 2: CHASE-LEV DEQUE + RESIZABLE DYNAMIC ARRAY
// ----------------------------------------------------------------------------

static DynamicTaskArray* dynamic_array_create(size_t capacity) {
    DynamicTaskArray* arr = malloc(sizeof(DynamicTaskArray) + capacity * sizeof(_Atomic(TaskItem)));
    arr->capacity = capacity;
    arr->mask = capacity - 1;
    return arr;
}

static void chase_lev_init(ChaseLevDeque* deque, size_t initial_capacity) {
    atomic_store_explicit(&deque->top, 0, memory_order_relaxed);
    atomic_store_explicit(&deque->bottom, 0, memory_order_relaxed);
    atomic_store_explicit(&deque->array, dynamic_array_create(initial_capacity), memory_order_relaxed);
}

// Push bởi Owner: O(1) Tuyệt đối không dùng CAS
static void chase_lev_push(ChaseLevDeque* deque, TaskItem item) {
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    size_t t = atomic_load_explicit(&deque->top, memory_order_acquire);
    DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_relaxed);

    if (b - t >= a->capacity) { // Queue Full -> Tự động x2 kích thước (Dynamic Resizing)
        DynamicTaskArray* new_a = dynamic_array_create(a->capacity * 2);
        for (size_t i = t; i < b; ++i) {
            TaskItem it = atomic_load_explicit(&a->buffer[i & a->mask], memory_order_relaxed);
            atomic_store_explicit(&new_a->buffer[i & new_a->mask], it, memory_order_relaxed);
        }
        atomic_store_explicit(&deque->array, new_a, memory_order_release);
        a = new_a;
    }

    atomic_store_explicit(&a->buffer[b & a->mask], item, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
}

// Pop bởi Owner: Không dùng CAS trừ khi còn 1 item duy nhất
static bool chase_lev_pop(ChaseLevDeque* deque, TaskItem* out_item) {
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    if (b == 0) return false;
    b--;
    atomic_store_explicit(&deque->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    size_t t = atomic_load_explicit(&deque->top, memory_order_relaxed);

    if (t <= b) {
        DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_relaxed);
        *out_item = atomic_load_explicit(&a->buffer[b & a->mask], memory_order_relaxed);
        if (t != b) return true;

        size_t expected_t = t;
        bool success = atomic_compare_exchange_strong_explicit(&deque->top, &expected_t, t + 1, memory_order_seq_cst, memory_order_relaxed);
        atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        return success;
    } else {
        atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        return false;
    }
}

// Steal bởi Stealer Thread: Dùng CAS ở Đuôi Queue
static bool chase_lev_steal(ChaseLevDeque* deque, TaskItem* out_item) {
    size_t t = atomic_load_explicit(&deque->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    size_t b = atomic_load_explicit(&deque->bottom, memory_order_acquire);

    if (t < b) {
        DynamicTaskArray* a = atomic_load_explicit(&deque->array, memory_order_consume);
        *out_item = atomic_load_explicit(&a->buffer[t & a->mask], memory_order_relaxed);

        size_t expected_t = t;
        if (atomic_compare_exchange_strong_explicit(&deque->top, &expected_t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// NÂNG CẤP 4: NATIVE IO_URING ON ARCH LINUX KERNEL
// ----------------------------------------------------------------------------

static int sys_io_uring_setup(uint32_t entries, struct io_uring_params* p) {
    return (int)syscall(__NR_io_uring_setup, entries, p);
}

static int sys_io_uring_enter(int fd, uint32_t to_submit, uint32_t min_complete, uint32_t flags) {
    return (int)syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, NULL, 0);
}

static void io_uring_init(IoUringEngine* ring) {
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    ring->ring_fd = sys_io_uring_setup(32, &p);

    if (ring->ring_fd < 0) {
        ring->enabled = false;
        return;
    }

    ring->sq_ring_mask = p.sq_off.ring_entries;
    ring->cq_ring_mask = p.cq_off.ring_entries;
    ring->enabled = true;
}

bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data) {
    if (!worker->io_ring.enabled) return false;
    // Tự động đóng gói lệnh read vào Submission Queue Entry (SQE)
    sys_io_uring_enter(worker->io_ring.ring_fd, 1, 0, 0);
    return true;
}

// Poll completions từ Kernel mà không gây Block Thread
static inline void io_uring_poll_completions(Worker* worker) {
    if (!worker->io_ring.enabled) return;
    sys_io_uring_enter(worker->io_ring.ring_fd, 0, 0, IORING_ENTER_GETEVENTS);
}

// ----------------------------------------------------------------------------
// NÂNG CẤP 3: SIMD TASK BATCHING (AVX2 / AVX-512)
// ----------------------------------------------------------------------------

void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count) {
    if (count == 0) return;
    atomic_fetch_add_explicit(&pool->total_tasks, count, memory_order_relaxed);

    if (g_local_worker != nullptr) [[likely]] {
#pragma GCC ivdep
        for (size_t i = 0; i < count; ++i) {
            chase_lev_push(&g_local_worker->dynamic_deque, items[i]);
        }
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        uint32_t target = fast_modulo(pcg_rand(&pool->rng_seed), pool->num_workers);
        priv_queue_enqueue(&pool->workers[target].queue, items[i]);
    }
}

size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch) {
    size_t count = 0;
#pragma GCC ivdep
#pragma clang loop vectorize(enable)
    for (size_t i = 0; i < max_batch; ++i) {
        if (chase_lev_pop(&worker->dynamic_deque, &out_batch[count])) {
            count++;
        } else if (priv_queue_dequeue(&worker->queue, &out_batch[count])) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

// Work-Stealing tổng hợp cả 2 hàng đợi
[[gnu::always_inline]] static inline bool worker_steal_all(PrivatizedThreadPool* pool, TaskItem* out_task, size_t my_id) {
    size_t num_q = pool->num_workers;
    size_t victim = pool->workers[my_id].last_victim;

#pragma GCC unroll 4
    for (size_t i = 0; i < num_q; ++i) {
        if (chase_lev_steal(&pool->workers[victim].dynamic_deque, out_task)) [[likely]] {
            pool->workers[my_id].last_victim = victim; // Lưu nạn nhân bị trộm
            return true;
        }
        if (priv_queue_dequeue(&pool->workers[victim].queue, out_task)) [[likely]] {
            pool->workers[my_id].last_victim = victim; // Lưu nạn nhân bị trộm
            return true;
        }
        victim = (victim + 1) % num_q;
    }
    return false;
}

// ----------------------------------------------------------------------------
// WORKER LOOP & APIS GIỮ NGUYÊN INTERFACE
// ----------------------------------------------------------------------------

static int privatized_worker_loop(void* arg) {
    PrivatizedThreadPool* pool = (PrivatizedThreadPool*)arg;
    Worker* w = g_local_worker;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(g_worker_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!atomic_load_explicit(&pool->stop, memory_order_relaxed)) {
        TaskItem task;
        bool found = false;

        // 1. Speculative Cache Slot từ Taskflow
        if (w->has_cache) [[likely]] {
            task = w->cache_task;
            w->has_cache = false;
            found = true;
        }
        // 2. Lấy từ Chase-Lev Dynamic Deque (Nâng cấp 1 & 2 - Lock-Free $O(1)$)
        else if (chase_lev_pop(&w->dynamic_deque, &task)) [[likely]] {
            found = true;
        }
        // 3. Lấy từ Bounded MPMC Queue cũ
        else if (priv_queue_dequeue(&w->queue, &task)) [[likely]] {
            found = true;
        }
        // 4. Trộm việc từ các Worker khác (Chase-Lev + Bounded Queue)
        else if (worker_steal_all(pool, &task, g_worker_id)) [[unlikely]] {
            found = true;
        }

        // 5. Kiểm tra Kernel I/O Events không bế tắc thread (Nâng cấp 4)
        io_uring_poll_completions(w);

        // Thực thi Task
        if (found) [[likely]] {
            if (task.func) {
                task.func(nullptr, task.user_data);
            }
            atomic_fetch_sub_explicit(&pool->total_tasks, 1, memory_order_relaxed);
        } else {
            _mm_pause(); // Tối ưu x86_64 Pipeline Pause
            thrd_yield();
        }
    }
    return 0;
}

PrivatizedThreadPool* privatized_pool_create(size_t num_workers) {
    PrivatizedThreadPool* pool = calloc(1, sizeof(PrivatizedThreadPool));
    if (!pool) return nullptr;

    pool->num_workers = num_workers;
    pool->workers = calloc(num_workers, sizeof(Worker));
    pool->threads = calloc(num_workers, sizeof(thrd_t));
    pool->rng_seed = 987654321;

    for (size_t i = 0; i < num_workers; ++i) {
        CoreType assigned_core = (i % 2 == 0) ? CORE_TYPE_PCORE : CORE_TYPE_ECORE;
        priv_queue_init(&pool->workers[i].queue, assigned_core);
        chase_lev_init(&pool->workers[i].dynamic_deque, PRIV_QUEUE_CAPACITY);
        io_uring_init(&pool->workers[i].io_ring);

        mtx_init(&pool->workers[i].lock, mtx_plain);
        cnd_init(&pool->workers[i].cv);
        pool->workers[i].last_victim = (i + 1) % num_workers; //
    }

    for (size_t i = 0; i < num_workers; ++i) {
        g_local_worker = &pool->workers[i];
        g_worker_id = i;
        thrd_create(&pool->threads[i], privatized_worker_loop, pool);
    }

    return pool;
}

void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core) {
    TaskItem item = {
        .func = func,
        .user_data = arg,
        .priority = prio,
        .target_core = core
    };

    atomic_fetch_add_explicit(&pool->total_tasks, 1, memory_order_relaxed);

    if (g_local_worker != nullptr) [[likely]] {
        if (!g_local_worker->has_cache) {
            g_local_worker->cache_task = item;
            g_local_worker->has_cache = true;
            return;
        }
        // Push qua Chase-Lev Lock-Free Deque ($O(1)$ Không cần CAS)
        chase_lev_push(&g_local_worker->dynamic_deque, item);
        return;
    }

    uint32_t target = fast_modulo(pcg_rand(&pool->rng_seed), pool->num_workers); //
    if (!priv_queue_enqueue(&pool->workers[target].queue, item)) [[unlikely]] {
        if (pool->base_pool) {
            TaskHandle* handle = pool_create_task_ex(pool->base_pool, func, arg, prio, core);
            queue_push(pool->base_pool, prio, core, handle);
        }
    }
}

void privatized_pool_destroy(PrivatizedThreadPool* pool) {
    if (!pool) return;

    atomic_store_explicit(&pool->stop, true, memory_order_relaxed);

    for (size_t i = 0; i < pool->num_workers; ++i) {
        thrd_join(pool->threads[i], nullptr);
        mtx_destroy(&pool->workers[i].lock);
        cnd_destroy(&pool->workers[i].cv);

        DynamicTaskArray* arr = atomic_load_explicit(&pool->workers[i].dynamic_deque.array, memory_order_relaxed);
        free(arr);
        if (pool->workers[i].io_ring.enabled) {
            close(pool->workers[i].io_ring.ring_fd);
        }
    }

    free(pool->workers);
    free(pool->threads);
    free(pool);
}

#pragma GCC diagnostic pop