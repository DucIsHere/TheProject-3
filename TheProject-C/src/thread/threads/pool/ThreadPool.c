#include "ThreadPool.h"

#include <immintrin.h>
#include <stddef.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

thread_local size_t g_workder_thread_id = 0;

static void pool_bind_thread_affinity(size_t thread_id)
{
     cpu_set_t cpuset;
     CPUZERO(&cpuset);
     CPU_SET(thread_id, &cpuset);
     pthread_setaffinity_np(pthread_self(), sizeof(&cpuset), &cpuset);
}

static void init_info(Thrd* pool)
{
     for (size_t i = 0; i < TOTAL_HARDWARE_THREAD; i++)
     {
          pool->info[i].thread_id = i;
          if (i < 16)
          {
               pool->info[i].core_id = (i / 2) + i;
               pool->info[i].core_type = CORE_TYPE_PCORE;
               pool->info[i].is_smt_thread = (i % 2 != 0);
          } else {
               pool->info[i].core_id = (i - 16) + 9;
               pool->info[i].core_type = CORE_TYPE_ECORE;
               pool->info[i].is_smt_thread = false;
          }
     }
}

static void init_mpmc_queue(MPMCQueue* q)
{
     atomic_store_explicit(&q->head, 0, memory_order_realxed);
     atomic_store_explicit(&q->tail, 0, memory_order_relaxed);
     for (size_t i = 0; i < TASK_QUEUE_CAPACITY; i++)
     {
          atomic_store_explicit(&q->ring[i].squence, i, memory_order_relaxed);
          q->ring[i].task = nullptr;
     }
}

static inline TaskHandle* arena_alloc_task(Thrd* pool)
{
     const size_t arena_capacity = TASK_QUEUE_CAPACITY * 2;

     while (true)
     {
          size_t idx = atomic_fetch_add_explicit(&pool->memory_arena.arena_idx, 1, memory_order_relaxed);
          size_t slot = idx % arena_capacity;
          TaskHandle* task = &pool->memory_arena.pool_arena[slot];

          TaskState expected = TASK_STATE_FREE;
          // Bật cờ chiếm Slot an toàn, tránh bị ghi đè dữ liệu đang chạy
          if (atomic_compare_exchange_strong_explicit(&task->state, &expected, TASK_STATE_WAITING_DEP,
                                                    memory_order_acq_rel, memory_order_relaxed)) 
          {
               // Khởi tạo lại thông số cho Task Node
               task->func = nullptr;
               task->arg = nullptr;
               task->counter = nullptr;
               task->future_result = nullptr;
               atomic_store_explicit(&task->dependency_count, 0, memory_order_relaxed);
               atomic_store_explicit(&task->dependents_head, nullptr, memory_order_relaxed);
               task->parent = nullptr;
               atomic_store_explicit(&task->active, 0, memory_order_relaxed);
               task->pool = pool;

               return task;
          }

          // Nếu Arena đầy (chưa Slot nào FREE), nhường CPU để tránh deadlock
          sched_yield();
     }
}

static inline void arena_free_task(TaskHandle* task)
{
     DependencyNode* curr = atomic_change_explicit(&task->dependency_head, nullptr, memory_order_acquire);
     while (curr)
     {
          DependencyNode* next = curr->next;
          free (curr);
          curr = next;
     }
     atomic_store_explicit(&task->state, TASK_STATE_FREE, memory_order_release);
}

static inline bool queue_push(Thrd* pool, TaskPriority prio, CoreType core_type, TaskHandle* task)
{
    MPMCQueue * q = (core_type == CORE_TYPE_PCORE) ? &pool->p_queues[prio] : &pool->e_queues[prio];
     QueueCell* cell;
     size_t pos = atomic_load_explicit(&q->tail, memory_order_relaxed);

     while (true)
     {
          cell = &q->ring[pos % TASK_QUEUE_CAPACITY];
          size_t seq = atomic_load_explicit(&cell->squence, memory_order_acquire);
          intptr_t diff = (intptr_t)seq - (intptr_t)pos;

          if (diff == 0)
          {
               if (atomic_compare_exchange_weak_explicit(&q->tail, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed))
               {
                    break;
               }
          } else if (diff < 0)
          {
               return false;
          } else {
               return atomic_load_explicit(&q->tail, memory_order_relaxed);
          }
     }

     cell->task = task;
     atomic_store_explicit(&cell->squence, pos + 1, memory_order_relaxed);
     return true;
}

static inline TaskHandle* queue_pop(Thrd* pool, TaskPriority prio, CoreType core_type) 
{
     MPMCQueue* q = (core_type == CORE_TYPE_PCORE) ? &pool->p_queues[prio] : &pool->e_queues[prio];
     QueueCell* cell;
     size_t pos = atomic_load_explicit(&q->head, memory_order_relaxed);

     while (true) 
     {
         cell = &q->ring[pos % TASK_QUEUE_CAPACITY];
         size_t seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);
         intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

          if (diff == 0) {
             if (atomic_compare_exchange_weak_explicit(&q->head, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed)) 
             {
               break;
             }
          } else if (diff < 0) 
          {
             return nullptr; // Queue rỗng
          } else {
             pos = atomic_load_explicit(&q->head, memory_order_relaxed);
          }
     }

     TaskHandle* task = cell->task;
     atomic_store_explicit(&cell->sequence, pos + TASK_QUEUE_CAPACITY, memory_order_release);
     return task;
}

// =============================================================================
// TASK EXECUTION ENGINE (DAG RESOLUTION)
// =============================================================================
static inline void execute_task_handle(TaskHandle* task) 
{
    TaskHandle* current = task;

    while (current != nullptr)
    {

    atomic_store_explicit(&task->state, TASK_STATE_EXECUTING, memory_order_release);

    void* res = nullptr;
    if (task->func) 
    {
        res = task->func(task, task->arg);
    }

    if (task->future_result) 
    {
        atomic_store_explicit(task->future_result, res, memory_order_release);
    }

    if (task->counter) 
    {
        atomic_fetch_sub_explicit(task->counter, 1, memory_order_release);
    }

    atomic_store_explicit(&task->state, TASK_STATE_FINISHED, memory_order_release);

    // Kích hoạt các Task con phụ thuộc trong DAG (Fan-out)
    DependencyNode* curr = atomic_load_explicit(&task->dependents_head, memory_order_acquire);
    TaskHandle* immediate_next = nullptr;

    while (curr) {
        TaskHandle* dep = curr->task;
        if (atomic_fetch_sub_explicit(&dep->dependency_count, 1, memory_order_acq_rel) == 1) {
            atomic_store_explicit(&dep->state, TASK_STATE_READY, memory_order_release);
            queue_push(task->pool, dep->priority, dep->target_core, dep);
            cnd_signal(&task->pool->wake_signals);
        }
        curr = curr->next;
    }

    if (task->parent) {
        atomic_fetch_sub_explicit(&task->parent->active, 1, memory_order_release);
    }

    // Trả lại tài nguyên cho Arena sau khi hoàn tất
    arena_free_task(task);
    current = immediate_next;
}
}

// =============================================================================
// WORKER LOOP (ASYMMETRIC WORK-STEALING)
// =============================================================================
static int worker_thread_entry(void* arg) {
    typedef struct { Thrd* pool; size_t id; } WorkerInit;
    WorkerInit* init = (WorkerInit*)arg;
    Thrd* pool = init->pool;
    size_t thread_id = init->id;
    g_worker_thread_id = thread_id;
    free(init);

    pool_bind_thread_affinity(thread_id);
    bool is_p_core = (thread_id < 16);

    while (!atomic_load_explicit(&pool->shutdown, memory_order_relaxed)) {
        
        // 1. CHẾ ĐỘ TÍNH TOÁN SONG SONG PDE
        if (atomic_load_explicit(&pool->pde.pde_mode, memory_order_acquire)) {
            size_t total = pool->pde.total_elements;
            size_t chunk = (total + TOTAL_HARDWARE_THREADS - 1) / TOTAL_HARDWARE_THREADS;
            size_t start = thread_id * chunk;
            size_t end = (start + chunk > total) ? total : start + chunk;

            if (start < total && pool->pde.func) {
                ParallelRange range = {
                    .start_idx = start,
                    .end_idx = end,
                    .thread_id = thread_id,
                    .user_data = pool->pde.user_data,
                    .pool = pool
                };
                pool->pde.func(&range);
            }

            if (atomic_fetch_sub_explicit(&pool->pde.pending_workers, 1, memory_order_acq_rel) == 1) {
                atomic_store_explicit(&pool->pde.pde_mode, false, memory_order_release);
                cnd_signal(&pool->pde.dispath_signals);
            }

            mtx_lock(&pool->pde.dispath_lock);
            while (atomic_load_explicit(&pool->pde.pde_mode, memory_order_relaxed)) {
                cnd_wait(&pool->pde.dispath_signals, &pool->pde.dispath_lock);
            }
            mtx_unlock(&pool->pde.dispath_lock);
            continue;
        }

        // 2. CHẾ ĐỘ WORK-STEALING ĐỘNG (DAG & WORK STEALING)
        TaskHandle* task = nullptr;

        if (is_p_core) {
            for (int p = 0; p < TASK_PRIO_COUNT; p++) {
                task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_PCORE);
                if (task) break;
            }
            if (!task) {
                for (int p = 0; p < TASK_PRIO_COUNT; p++) {
                    task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_ECORE);
                    if (task) break;
                }
            }
        } else {
            for (int p = TASK_PRIO_LOW; p >= TASK_PRIO_CRITICAL; p--) {
                task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_ECORE);
                if (task) break;
            }
            if (!task) {
                for (int p = TASK_PRIO_LOW; p >= TASK_PRIO_CRITICAL; p--) {
                    task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_PCORE);
                    if (task) break;
                }
            }
        }

        if (task) {
            execute_task_handle(task);
        } else {
            if (is_p_core)
            {
                uint32_t spin = 0;
                while (spin < 2500)
                {
                    _mm_pause();

                    for (int p = 0; p < TASK_PRIO_COUNT; p++)
                    {
                        task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_PCORE);
                        if (task) break;
                    }

                    if (task) break;
                    spin++;
                }

                if (task)
                {
                    execute_task_handle(task);
                    continue;
                }
            }

            mtx_lock(&pool->wake_lock);
            cnd_wait(&pool->wake_signals);
            cnd_wait(&pool->wake_lock);
            mtx_unlock(&pool->wake_lock);
        }
    }
    return 0;
}

// =============================================================================
// PUBLIC APIS
// =============================================================================
Thrd* pool_make(void) {
    return pool_create(TOTAL_HARDWARE_THREADS);
}

Thrd* pool_create(size_t thread_count) {
    Thrd* pool = (Thrd*)calloc(1, sizeof(Thrd));
    pool->thread_count = (thread_count > TOTAL_HARDWARE_THREADS) ? TOTAL_HARDWARE_THREADS : thread_count;

    init_topology_info(pool);
    
    // Khởi tạo hàng đợi MPMC Lock-free
    for (int p = 0; p < TASK_PRIO_COUNT; p++) {
        init_mpmc_queue(&pool->p_queues[p]);
        init_mpmc_queue(&pool->e_queues[p]);
    }

    // Đánh dấu toàn bộ Slot trong Arena là FREE
    for (size_t i = 0; i < TASK_QUEUE_CAPACITY * 2; i++) {
        atomic_store_explicit(&pool->memory_arena.pool_arena[i].state, TASK_STATE_FREE, memory_order_relaxed);
    }

    cnd_init(&pool->wake_signals);
    mtx_init(&pool->wake_lock, mtx_plain);
    cnd_init(&pool->pde.dispath_signals);
    mtx_init(&pool->pde.dispath_lock, mtx_plain);

    for (size_t i = 0; i < pool->thread_count; i++) {
        typedef struct { Thrd* pool; size_t id; } WorkerInit;
        WorkerInit* init = (WorkerInit*)malloc(sizeof(WorkerInit));
        init->pool = pool;
        init->id = i;
        thrd_create(&pool->threads[i], worker_thread_entry, init);
    }
    return pool;
}

void pool_destroy(Thrd* pool) {
    atomic_store_explicit(&pool->shutdown, true, memory_order_release);
    cnd_broadcast(&pool->wake_signals);
    cnd_broadcast(&pool->pde.dispath_signals);

    for (size_t i = 0; i < pool->thread_count; i++) {
        thrd_join(pool->threads[i], nullptr);
    }

    cnd_destroy(&pool->wake_signals);
    mtx_destroy(&pool->wake_lock);
    cnd_destroy(&pool->pde.dispath_signals);
    mtx_destroy(&pool->pde.dispath_lock);
    free(pool);
}

typedef struct {
    void (*simple_func)(void*);
    void* user_args;
} SimpleTaskAdapter;

static void* simple_task_wrapper(TaskHandle* self, void* arg) {
    SimpleTaskAdapter* adapter = (SimpleTaskAdapter*)arg;
    if (adapter->simple_func) {
        adapter->simple_func(adapter->user_args);
    }
    free(adapter);
    return nullptr;
}

void pool_submit(Thrd* pool, void (*func)(void*), void* args, _Atomic(size_t)* counter) {
    SimpleTaskAdapter* adapter = (SimpleTaskAdapter*)malloc(sizeof(SimpleTaskAdapter));
    adapter->simple_func = func;
    adapter->user_args = args;

    TaskHandle* task = pool_create_task_ex(pool, simple_task_wrapper, adapter, TASK_PRIO_NORMAL, CORE_TYPE_PCORE);
    pool_submit_task(task, counter, nullptr);
}

void pool_wait(_Atomic(size_t)* counter) {
    if (!counter) return;
    while (atomic_load_explicit(counter, memory_order_acquire) > 0) {
        sched_yield();
    }
}

TaskHandle* pool_create_task_ex(Thrd* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType target) {
    TaskHandle* task = arena_alloc_task(pool);
    task->func = func;
    task->arg = arg;
    task->priority = prio;
    task->target_core = target;
    return task;
}

// Thêm kết nối DAG phụ thuộc động
bool pool_add_dependency(TaskHandle* parent, TaskHandle* dependent) {
    if (!parent || !dependent) return false;

    DependencyNode* new_node = (DependencyNode*)malloc(sizeof(DependencyNode));
    new_node->task = dependent;

    atomic_fetch_add_explicit(&dependent->dependency_count, 1, memory_order_relaxed);

    // Thêm Node vào danh sách liên kết Lock-Free của Task parent
    DependencyNode* old_head = atomic_load_explicit(&parent->dependents_head, memory_order_relaxed);
    do {
        new_node->next = old_head;
    } while (!atomic_compare_exchange_weak_explicit(&parent->dependents_head, &old_head, new_node,
                                                    memory_order_release, memory_order_relaxed));
    return true;
}

void pool_submit_task(TaskHandle* task, _Atomic(size_t)* counter, _Atomic(void*)* future_out) {
    task->counter = counter;
    task->future_result = future_out;
    if (counter) atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);

    // Nếu không còn phụ thuộc cha (DAG root node), đẩy ngay vào Queue
    if (atomic_load_explicit(&task->dependency_count, memory_order_acquire) == 0) {
        atomic_store_explicit(&task->state, TASK_STATE_READY, memory_order_release);
        queue_push(task->pool, task->priority, task->target_core, task);
        cnd_signal(&task->pool->wake_signals);
    }
}

void pool_wait_active(Thrd* pool, _Atomic(size_t)* counter) {
    if (!counter) return;

    while (atomic_load_explicit(counter, memory_order_acquire) > 0) {
        TaskHandle* task = nullptr;
        for (int p = 0; p < TASK_PRIO_COUNT; p++) {
            task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_PCORE);
            if (!task) task = queue_pop(pool, (TaskPriority)p, CORE_TYPE_ECORE);
            if (task) break;
        }

        if (task) {
            execute_task_handle(task);
        } else {
            sched_yield();
        }
    }
}

void pool_fork_active(TaskHandle* parent_task, TaskFunc sub_func, void** arg_array, size_t subtask_count) {
    Thrd* pool = parent_task->pool;
    atomic_store_explicit(&parent_task->active, subtask_count, memory_order_relaxed);

    for (size_t i = 0; i < subtask_count; i++) {
        TaskHandle* sub = pool_create_task_ex(pool, sub_func, arg_array[i], parent_task->priority, CORE_TYPE_PCORE);
        sub->parent = parent_task;
        atomic_store_explicit(&sub->state, TASK_STATE_READY, memory_order_release);
        queue_push(pool, sub->priority, sub->target_core, sub);
    }

    cnd_broadcast(&pool->wake_signals);

    while (atomic_load_explicit(&parent_task->active, memory_order_acquire) > 0) {
        TaskHandle* st = queue_pop(pool, parent_task->priority, CORE_TYPE_PCORE);
        if (st) {
            execute_task_handle(st);
        } else {
            sched_yield();
        }
    }
}

void pool_pde_parallel(Thrd* pool, size_t total_element, ParallelFunc func, void* user_data) {
    mtx_lock(&pool->pde.dispath_lock);
    
    pool->pde.func = func;
    pool->pde.user_data = user_data;
    pool->pde.total_elements = total_element;
    pool->pde.barrier.total_thread = pool->thread_count;
    atomic_store_explicit(&pool->pde.barrier.in_barrier, 0, memory_order_relaxed);
    atomic_store_explicit(&pool->pde.barrier.out_barrier, 0, memory_order_relaxed);

    atomic_store_explicit(&pool->pde.pending_workers, pool->thread_count, memory_order_relaxed);
    atomic_store_explicit(&pool->pde.pde_mode, true, memory_order_release);

    cnd_broadcast(&pool->wake_signals);

    while (atomic_load_explicit(&pool->pde.pde_mode, memory_order_acquire)) {
        cnd_wait(&pool->pde.dispath_signals, &pool->pde.dispath_lock);
    }

    mtx_unlock(&pool->pde.dispath_lock);
}

void pool_pde_barrier(ParallelRange* range) {
    Barrier* b = &range->pool->pde.barrier;
    size_t total = b->total_thread;

    size_t arrival = atomic_fetch_add_explicit(&b->in_barrier, 1, memory_order_acq_rel);
    if (arrival == total - 1) {
        atomic_store_explicit(&b->out_barrier, 0, memory_order_release);
        atomic_store_explicit(&b->in_barrier, 0, memory_order_release);
    } else {
        while (atomic_load_explicit(&b->in_barrier, memory_order_acquire) != 0) {
            sched_yield();
        }
    }
}

FFIBridgeContext* ffi_bridge_create_context(void* java_in, void* java_out, size_t size)
{
    FFIBridgeContext* ctx = (FFIBridgeContext*)malloc(sizeof(FFIBridgeContext));
    ctx->java_in_buffer = java_in;
    ctx->java_out_buffer = java_out;
    ctx->data_size = size;

    uintptr_t raw_ptr = (uintptr_t)java_in;
    uintptr_t aligned_ptr = (raw_ptr + 63) & ~63;

    ctx->pcore_aligned_ptr = (void*)aligned_ptr;
    ctx->flags = 0;

    return ctx;
}

void ffi_bridge_free_context(FFIBridgeContext* ctx)
{
    if (ctx) free(ctx);
}

void pool_submit_ffi_dag_pipeline(Thrd* pool, FFIBridgeContext* ffi_ctx, TaskHandle pcore_compute_func, _Atomic(size_t)* counter)
{
    TaskHandle* prep_task = pool_create_task_ex(pool, nullptr, ffi_ctx, TASK_PRIO_HIGH, CORE_TYPE_ECORE);
    TaskHandle* comp_task = pool_create_task_ex(pool, pcore_compute_func, ffi_ctx, TASK_PRIO_CRITICAL, CORE_TYPE_PCORE);
    pool_add_dependency(prep_task, comp_task);
    pool_submit_task(prep_task, counter, nullptr);
    pool_submit_task(comp_task, counter, nullptr);
}
