//
// Created by BaoZhuang on 30/10/2026.
//
// Hybrid scheduler:
//   * lock-free : MPMC queues, arena CAS allocation, dependency_count (DAG fan-in),
//                 submit hold, reference counting, counters/futex waits.
//   * mutex/cond: worker sleep/wake (wake_lock), PDE round serialisation (call_lock)
//                 and PDE completion signal (dispath_lock).
//

#include "ThreadPool.h"

/* SIZE_MAX / NULL mean that the current thread is not a pool worker. */
static _Thread_local size_t g_worker_thread_id = SIZE_MAX;
static _Thread_local Thrd*  g_worker_pool = NULL;
static _Thread_local uint32_t g_serial_pde_depth = 0;

_Static_assert(TASK_STATE_FREE == 0, "a zeroed arena must mean FREE");
_Static_assert(_Alignof(TaskHandle) >= CL, "TaskHandle must be cache-line aligned");
_Static_assert(_Alignof(MPMCQueue) >= CL, "MPMCQueue must be cache-line aligned");
_Static_assert(_Alignof(Thrd) >= CL, "Thrd must be cache-line aligned");

static INLINE bool on_pool_worker(const Thrd* pool) {
    return g_worker_pool == pool && g_worker_thread_id < pool->thread_count;
}

/* ------------------------------------------------------------------------- */
/* Native sync wrappers                                                      */
/* ------------------------------------------------------------------------- */

static INLINE void native_mutex_init(native_mutex_t* mtx) {
#if defined(PCCORE_OS_WINDOWS)
    InitializeCriticalSection(mtx);
#elif defined(PCCORE_OS_LINUX)
    pthread_mutex_init(mtx, NULL);
#endif
}

static INLINE void native_mutex_destroy(native_mutex_t* mtx) {
#if defined(PCCORE_OS_WINDOWS)
    DeleteCriticalSection(mtx);
#elif defined(PCCORE_OS_LINUX)
    pthread_mutex_destroy(mtx);
#endif
}

static INLINE void native_mutex_lock(native_mutex_t* mtx) {
#if defined(PCCORE_OS_WINDOWS)
    EnterCriticalSection(mtx);
#elif defined(PCCORE_OS_LINUX)
    pthread_mutex_lock(mtx);
#endif
}

static INLINE void native_mutex_unlock(native_mutex_t* mtx) {
#if defined(PCCORE_OS_WINDOWS)
    LeaveCriticalSection(mtx);
#elif defined(PCCORE_OS_LINUX)
    pthread_mutex_unlock(mtx);
#endif
}

static INLINE void native_cond_init(native_cond_t* cond) {
#if defined(PCCORE_OS_WINDOWS)
    InitializeConditionVariable(cond);
#elif defined(PCCORE_OS_LINUX)
    pthread_cond_init(cond, NULL);
#endif
}

static INLINE void native_cond_destroy(native_cond_t* cond) {
#if defined(PCCORE_OS_WINDOWS)
    (void)cond;
#elif defined(PCCORE_OS_LINUX)
    pthread_cond_destroy(cond);
#endif
}

static INLINE void native_cond_wait(native_cond_t* cond, native_mutex_t* mtx) {
#if defined(PCCORE_OS_WINDOWS)
    SleepConditionVariableCS(cond, mtx, INFINITE);
#elif defined(PCCORE_OS_LINUX)
    pthread_cond_wait(cond, mtx);
#endif
}

static INLINE void native_cond_signal(native_cond_t* cond) {
#if defined(PCCORE_OS_WINDOWS)
    WakeConditionVariable(cond);
#elif defined(PCCORE_OS_LINUX)
    pthread_cond_signal(cond);
#endif
}

static INLINE void native_cond_broadcast(native_cond_t* cond) {
#if defined(PCCORE_OS_WINDOWS)
    WakeAllConditionVariable(cond);
#elif defined(PCCORE_OS_LINUX)
    pthread_cond_broadcast(cond);
#endif
}

static INLINE void cpu_relax(void) {
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
    _mm_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
    __asm__ volatile("yield" ::: "memory");
#endif
}

static INLINE void os_yield(void) {
#if defined(PCCORE_OS_LINUX)
    sched_yield();
#elif defined(PCCORE_OS_WINDOWS)
    SwitchToThread();
#endif
}

/* Worker sleep/wake: signalled under wake_lock so a sleeper can't miss it. */
static INLINE void wake_one_worker(Thrd* pool) {
    native_mutex_lock(&pool->wake_lock);
    native_cond_signal(&pool->wake_signals);
    native_mutex_unlock(&pool->wake_lock);
}

static void pool_wake_all(Thrd* pool) {
    native_mutex_lock(&pool->wake_lock);
    native_cond_broadcast(&pool->wake_signals);
    native_mutex_unlock(&pool->wake_lock);
}

/* ------------------------------------------------------------------------- */
/* Topology / affinity                                                       */
/* ------------------------------------------------------------------------- */

void pool_bind_thread_affinity(size_t core_id) {
#if defined(PCCORE_OS_WINDOWS)
    if (core_id >= sizeof(DWORD_PTR) * CHAR_BIT) return;
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = ((DWORD_PTR)1) << core_id;
    (void)SetThreadAffinityMask(thread, mask);
#elif defined(PCCORE_OS_LINUX)
    if (core_id >= CPU_SETSIZE) return;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core_id, &mask);
    pthread_t current_thread = pthread_self();
    (void)pthread_setaffinity_np(current_thread, sizeof(mask), &mask);
#endif
}

static void init_topology_info(Thrd* pool) {
    /*
     * The P/E classification assumes the common 8P+8E logical-CPU ordering.
     * On Linux, affinity uses the process' actually allowed CPU IDs rather than
     * assuming that the allowed set starts at CPU 0.
     */
#if defined(PCCORE_OS_LINUX)
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    const bool have_allowed_set =
        (sched_getaffinity(0, sizeof(allowed), &allowed) == 0);
    size_t next_cpu = 0;
#endif

    for (size_t i = 0; i < TOTAL_HARDWARE_THREADS; ++i) {
        pool->info[i].thread_id = i;
#if defined(PCCORE_OS_LINUX)
        if (have_allowed_set) {
            while (next_cpu < CPU_SETSIZE && !CPU_ISSET(next_cpu, &allowed))
                ++next_cpu;
            pool->info[i].core_id = (next_cpu < CPU_SETSIZE) ? next_cpu++ : i;
        } else {
            pool->info[i].core_id = i;
        }
#else
        pool->info[i].core_id = i;
#endif
        pool->info[i].core_type = (i < 16) ? CORE_TYPE_PCORE : CORE_TYPE_ECORE;
        pool->info[i].is_smt_thread = (i < 16) && ((i & 1u) != 0);
        pool->info[i].os_tid = 0;
    }
}

/* ------------------------------------------------------------------------- */
/* Lock-free MPMC queue (Vyukov bounded queue)                               */
/* ------------------------------------------------------------------------- */

static void init_mpmc_queue(MPMCQueue* q) {
    atomic_init(ATOMIC_PTR(&q->head), 0);
    atomic_init(ATOMIC_PTR(&q->tail), 0);
    for (size_t i = 0; i < TQ_CAPACITY; ++i) {
        atomic_init(ATOMIC_PTR(&q->ring[i].squence), i);
        q->ring[i].task = NULL;
    }
}

static INLINE bool queue_push(Thrd* pool, TaskPriority prio, CoreType core_type, TaskHandle* task) {
    MPMCQueue* q = (core_type == CORE_TYPE_PCORE)
        ? &pool->p_queue[prio]
        : &pool->e_queue[prio];

    QueueCell* cell;
    size_t pos = atomic_load_explicit(ATOMIC_PTR(&q->tail), memory_order_relaxed);
    uint32_t spin_count = 0;

    for (;;) {
        cell = &q->ring[pos % TQ_CAPACITY];
        const size_t seq = atomic_load_explicit(ATOMIC_PTR(&cell->squence), memory_order_acquire);
        const intptr_t diff = (intptr_t)seq - (intptr_t)pos;

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(
                    ATOMIC_PTR(&q->tail), &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                break;
            }
        } else if (diff < 0) {
            /* Queue full: back off without taking a lock. */
            if (++spin_count > 2048) {
                os_yield();
                spin_count = 0;
            } else {
                cpu_relax();
            }
            pos = atomic_load_explicit(ATOMIC_PTR(&q->tail), memory_order_relaxed);
        } else {
            pos = atomic_load_explicit(ATOMIC_PTR(&q->tail), memory_order_relaxed);
        }
    }

    cell->task = task;
    atomic_store_explicit(ATOMIC_PTR(&cell->squence), pos + 1, memory_order_release);
    return true;
}

static INLINE TaskHandle* queue_pop(Thrd* pool, TaskPriority prio, CoreType core_type) {
    MPMCQueue* q = (core_type == CORE_TYPE_PCORE)
        ? &pool->p_queue[prio]
        : &pool->e_queue[prio];

    QueueCell* cell;
    size_t pos = atomic_load_explicit(ATOMIC_PTR(&q->head), memory_order_relaxed);

    for (;;) {
        cell = &q->ring[pos % TQ_CAPACITY];
        const size_t seq = atomic_load_explicit(ATOMIC_PTR(&cell->squence), memory_order_acquire);
        const intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(
                    ATOMIC_PTR(&q->head), &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                break;
            }
        } else if (diff < 0) {
            return NULL; /* Empty */
        } else {
            pos = atomic_load_explicit(ATOMIC_PTR(&q->head), memory_order_relaxed);
        }
    }

    TaskHandle* task = cell->task;
    atomic_store_explicit(ATOMIC_PTR(&cell->squence), pos + TQ_CAPACITY, memory_order_release);
    return task;
}

/* ------------------------------------------------------------------------- */
/* Task arena + reference counting                                           */
/* ------------------------------------------------------------------------- */

/*
 * Lifetime of a TaskHandle (`active` is a reference count):
 *   1  creator/execution reference   (dropped when the task finishes executing)
 *  +1  temporary submit reference    (dropped at the end of pool_submit_task)
 *  +1  per forked child              (dropped when the child finishes)
 *  +1  per dependency edge in flight (dropped after fan-out touches the dependent)
 * The slot returns to the arena when the count reaches 0.
 *
 * dependency_count starts at 1: that is the "submission hold". It is released by
 * pool_submit_task, and whoever moves the count 1 -> 0 schedules the task
 * exactly once. This is the lock-free DAG fan-in.
 */
static INLINE TaskHandle* arena_alloc_task(Thrd* pool) {
    const size_t arena_capacity = TQ_CAPACITY * 2;
    uint32_t spin_count = 0;

    for (;;) {
        const size_t idx = atomic_fetch_add_explicit(
            ATOMIC_PTR(&pool->memory_arena.arena_idx), 1, memory_order_relaxed);
        const size_t slot = idx % arena_capacity;
        TaskHandle* task = &pool->memory_arena.pool_arena[slot];

        TaskState expected = TASK_STATE_FREE;
        if (atomic_compare_exchange_strong_explicit(
                ATOMIC_PTR(&task->state), &expected, TASK_STATE_WAITING_DEP,
                memory_order_acq_rel, memory_order_relaxed)) {
            task->func = NULL;
            task->arg = NULL;
            task->counter = NULL;
            task->feature_result = NULL;
            task->parent = NULL;
            task->pool = pool;
            task->priority = TASK_PRIO_NORMAL;
            task->target_core = CORE_TYPE_PCORE;

            atomic_store_explicit(ATOMIC_PTR(&task->dependency_count), 1, memory_order_relaxed); /* submission hold */
            atomic_store_explicit(ATOMIC_PTR(&task->dependents_head), NULL, memory_order_relaxed);
            atomic_store_explicit(ATOMIC_PTR(&task->active), 1, memory_order_relaxed);
            atomic_store_explicit(ATOMIC_PTR(&task->submit_state), 0, memory_order_relaxed);
            return task;
        }

        if (++spin_count > 1024) {
            os_yield();
            spin_count = 0;
        } else {
            cpu_relax();
        }
    }
}

static INLINE void arena_free_task(TaskHandle* task) {
    DependencyNode* curr = atomic_exchange_explicit(
        ATOMIC_PTR(&task->dependents_head), NULL, memory_order_acq_rel);
    while (curr) {
        DependencyNode* next = curr->next;
        free(curr);
        curr = next;
    }

    task->func = NULL;
    task->arg = NULL;
    task->counter = NULL;
    task->feature_result = NULL;
    task->parent = NULL;
    task->pool = NULL;
    atomic_store_explicit(ATOMIC_PTR(&task->dependency_count), 0, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&task->active), 0, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&task->submit_state), 0, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&task->state), TASK_STATE_FREE, memory_order_release);
}

static INLINE void task_release_reference(TaskHandle* task) {
    if (atomic_fetch_sub_explicit(ATOMIC_PTR(&task->active), 1, memory_order_acq_rel) == 1) {
        arena_free_task(task);
    }
}

/* Caller must hold a reference and must have just taken dependency_count 1 -> 0. */
static INLINE void schedule_task(TaskHandle* task) {
    TaskState expected = TASK_STATE_WAITING_DEP;
    if (atomic_compare_exchange_strong_explicit(
            ATOMIC_PTR(&task->state), &expected, TASK_STATE_READY,
            memory_order_acq_rel, memory_order_acquire)) {
        Thrd* pool = task->pool;
        const TaskPriority priority = task->priority;
        const CoreType target = task->target_core;
        (void)queue_push(pool, priority, target, task);
        wake_one_worker(pool);
    }
}

static INLINE void execute_task_handle(TaskHandle* task) {
    if (!task) return;

    TaskState expected = TASK_STATE_READY;
    if (!atomic_compare_exchange_strong_explicit(
            ATOMIC_PTR(&task->state), &expected, TASK_STATE_EXCUTING,
            memory_order_acq_rel, memory_order_acquire)) {
        return; /* Prevent accidental double execution through the public pop API. */
    }

    void* result = task->func ? task->func(task, task->arg) : NULL;
    if (task->feature_result) {
        atomic_store_explicit(ATOMIC_PTR(task->feature_result), result, memory_order_release);
    }

    atomic_store_explicit(ATOMIC_PTR(&task->state), TASK_STATE_FINISHED, memory_order_release);

    /* Fan-out: release dependent tasks in the DAG (lock-free dependency_count). */
    DependencyNode* curr = atomic_load_explicit(
        ATOMIC_PTR(&task->dependents_head), memory_order_acquire);
    while (curr) {
        DependencyNode* next = curr->next;
        TaskHandle* dependent = curr->task;
        /* Keep the dependent alive while this edge is processed. */
        atomic_fetch_add_explicit(ATOMIC_PTR(&dependent->active), 1, memory_order_relaxed);
        if (atomic_fetch_sub_explicit(
                ATOMIC_PTR(&dependent->dependency_count), 1, memory_order_acq_rel) == 1) {
            schedule_task(dependent);
        }
        task_release_reference(dependent);
        curr = next;
    }

    /* Save all fields needed below before releasing this task's arena slot. */
    TaskHandle* parent = task->parent;
    pc_counter_t* counter = task->counter;

    /* Drop this child's reference to its parent, if it is a forked subtask. */
    if (parent) task_release_reference(parent);

    /* Drop this task's execution reference. Children/schedulers hold extra references. */
    task_release_reference(task);

    /* Counter decrement is deliberately last: zero means all task cleanup is done. */
    if (counter) {
        const int32_t prev = atomic_fetch_sub_explicit(ATOMIC_PTR(counter), 1, memory_order_release);
        if (prev == 1) sys_futex_wake(counter, INT32_MAX);
    }
}

/* ------------------------------------------------------------------------- */
/* Work finding                                                              */
/* ------------------------------------------------------------------------- */

typedef struct { Thrd* pool; size_t id; } WorkerInit;

static INLINE TaskHandle* pop_any_prio(Thrd* pool, CoreType core) {
    for (int p = 0; p < TASK_PRIO_COUNT; p++) {
        TaskHandle* t = queue_pop(pool, (TaskPriority)p, core);
        if (t) return t;
    }
    return NULL;
}

static INLINE TaskHandle* find_task(Thrd* pool, bool is_p_core) {
    CoreType own = is_p_core ? CORE_TYPE_PCORE : CORE_TYPE_ECORE;
    CoreType other = is_p_core ? CORE_TYPE_ECORE : CORE_TYPE_PCORE;
    TaskHandle* t = pop_any_prio(pool, own);
    return t ? t : pop_any_prio(pool, other);
}

/* ------------------------------------------------------------------------- */
/* PDE (parallel data execution) worker side                                 */
/* ------------------------------------------------------------------------- */

static INLINE void pde_run_chunk(Thrd* pool, size_t thread_id) {
    const size_t total = pool->pde.total_elements;
    const size_t chunk = 1 + (total - 1) / pool->thread_count;
    const size_t start = thread_id * chunk;
    const size_t end = (start >= total || chunk > total - start)
        ? total
        : start + chunk;

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

    if (atomic_fetch_sub_explicit(
            ATOMIC_PTR(&pool->pde.pending_workers), 1, memory_order_acq_rel) == 1) {
        native_mutex_lock(&pool->pde.dispath_lock);
        native_cond_broadcast(&pool->pde.dispath_signals);
        native_mutex_unlock(&pool->pde.dispath_lock);
    }
}

/* ------------------------------------------------------------------------- */
/* Worker thread                                                             */
/* ------------------------------------------------------------------------- */

#if defined(PCCORE_OS_WINDOWS)
static DWORD WINAPI worker_thread_entry(LPVOID arg)
#elif defined(PCCORE_OS_LINUX)
static void* worker_thread_entry(void* arg)
#endif
{
    WorkerInit* init = (WorkerInit*) arg;
    Thrd* pool = init->pool;
    size_t thread_id = init->id;
    g_worker_thread_id = thread_id;
    g_worker_pool = pool;
#if defined(PCCORE_OS_WINDOWS)
    pool->info[thread_id].os_tid = GetCurrentThreadId();
#elif defined(PCCORE_OS_LINUX)
    pool->info[thread_id].os_tid = (native_thread_id_t)syscall(SYS_gettid);
#endif
    free(init);

    pool_bind_thread_affinity(pool->info[thread_id].core_id);
    const bool is_p_core = (pool->info[thread_id].core_type == CORE_TYPE_PCORE);
    const CoreType own = is_p_core ? CORE_TYPE_PCORE : CORE_TYPE_ECORE;
    uint32_t pde_gen = 0;                      // generation starts at 0; first round makes it 1

    while (!atomic_load_explicit(ATOMIC_PTR(&pool->shutdown), memory_order_acquire)) {

        // 1. PDE round: run once per generation, then go back to normal work
        uint32_t gen = atomic_load_explicit(ATOMIC_PTR(&pool->pde.generation), memory_order_acquire);
        if (gen != pde_gen) {
            pde_gen = gen;
            pde_run_chunk(pool, thread_id);
            continue;
        }

        // 2. DAG / work stealing
        TaskHandle* task = find_task(pool, is_p_core);
        if (task) {
            execute_task_handle(task);
            continue;
        }

        // 3. P-cores spin briefly on their own queues before sleeping
        if (is_p_core) {
            for (uint32_t spin = 0; spin < 2500 && !task; spin++) {
                cpu_relax();
                task = pop_any_prio(pool, own);
            }
            if (task) {
                execute_task_handle(task);
                continue;
            }
        }

        // 4. Sleep. Every wake condition is rechecked under the lock.
        native_mutex_lock(&pool->wake_lock);
        if (atomic_load_explicit(ATOMIC_PTR(&pool->shutdown), memory_order_acquire) ||
            atomic_load_explicit(ATOMIC_PTR(&pool->pde.generation), memory_order_acquire) != pde_gen) {
            native_mutex_unlock(&pool->wake_lock);
            continue;
        }
        task = find_task(pool, is_p_core);
        if (task) {
            native_mutex_unlock(&pool->wake_lock);
            execute_task_handle(task);
            continue;
        }
        native_cond_wait(&pool->wake_signals, &pool->wake_lock);
        native_mutex_unlock(&pool->wake_lock);
    }

#if defined(PCCORE_OS_WINDOWS)
    return 0;
#elif defined(PCCORE_OS_LINUX)
    return NULL;
#endif
}

/* ------------------------------------------------------------------------- */
/* Allocation helpers                                                        */
/* ------------------------------------------------------------------------- */

static void* aligned_alloc_cl(size_t size) {
    if (size == 0) return NULL;
#if defined(PCCORE_OS_WINDOWS)
    return _aligned_malloc(size, CL);
#elif defined(PCCORE_OS_LINUX)
    void* ptr = NULL;
    if (posix_memalign(&ptr, CL, size) != 0) return NULL;
    return ptr;
#endif
}

static void aligned_free_cl(void* ptr) {
#if defined(PCCORE_OS_WINDOWS)
    _aligned_free(ptr);
#elif defined(PCCORE_OS_LINUX)
    free(ptr);
#endif
}

static void destroy_pool_sync(Thrd* pool) {
    native_cond_destroy(&pool->wake_signals);
    native_mutex_destroy(&pool->wake_lock);
    native_cond_destroy(&pool->pde.dispath_signals);
    native_mutex_destroy(&pool->pde.dispath_lock);
    native_mutex_destroy(&pool->pde.call_lock);
}

static void join_workers(Thrd* pool, size_t count) {
    for (size_t i = 0; i < count; ++i) {
#if defined(PCCORE_OS_WINDOWS)
        (void)WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
#elif defined(PCCORE_OS_LINUX)
        (void)pthread_join(pool->threads[i], NULL);
#endif
    }
}

/* ------------------------------------------------------------------------- */
/* Public API: lifecycle                                                     */
/* ------------------------------------------------------------------------- */

[[nodiscard]] Thrd* pool_create(size_t thread_count) {
    if (thread_count == 0 || thread_count > TOTAL_HARDWARE_THREADS)
        return NULL;

    Thrd* pool = aligned_alloc_cl(sizeof(*pool));
    if (!pool) return NULL;
    memset(pool, 0, sizeof(*pool));          // also marks every arena slot FREE

    pool->thread_count = thread_count;
    atomic_init(ATOMIC_PTR(&pool->shutdown), false);
    atomic_init(ATOMIC_PTR(&pool->memory_arena.arena_idx), 0);

    for (size_t p = 0; p < TASK_PRIO_COUNT; ++p) {
        init_mpmc_queue(&pool->p_queue[p]);
        init_mpmc_queue(&pool->e_queue[p]);
    }

    for (size_t i = 0; i < TQ_CAPACITY * 2; ++i) {
        TaskHandle* task = &pool->memory_arena.pool_arena[i];
        atomic_init(ATOMIC_PTR(&task->dependency_count), 0);
        atomic_init(ATOMIC_PTR(&task->dependents_head), NULL);
        atomic_init(ATOMIC_PTR(&task->active), 0);
        atomic_init(ATOMIC_PTR(&task->submit_state), 0);
        atomic_init(ATOMIC_PTR(&task->state), TASK_STATE_FREE);
    }

    atomic_init(ATOMIC_PTR(&pool->pde.pending_workers), 0);
    atomic_init(ATOMIC_PTR(&pool->pde.generation), 0);
    atomic_init(ATOMIC_PTR(&pool->pde.barrier.futex_val), 0);
    atomic_init(ATOMIC_PTR(&pool->pde.barrier.in_barrier), 0);
    atomic_init(ATOMIC_PTR(&pool->pde.barrier.out_barrier), 0);
    pool->pde.barrier.total_thread = 0;

    native_mutex_init(&pool->wake_lock);
    native_cond_init(&pool->wake_signals);
    native_mutex_init(&pool->pde.dispath_lock);
    native_cond_init(&pool->pde.dispath_signals);
    native_mutex_init(&pool->pde.call_lock);

    init_topology_info(pool);

    size_t created = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        WorkerInit* init = malloc(sizeof(*init));
        if (!init) break;
        init->pool = pool;
        init->id = i;

#if defined(PCCORE_OS_WINDOWS)
        pool->threads[i] = CreateThread(
            NULL, 0, worker_thread_entry, init, 0, NULL);
        if (!pool->threads[i]) {
            free(init);
            break;
        }
#elif defined(PCCORE_OS_LINUX)
        if (pthread_create(&pool->threads[i], NULL, worker_thread_entry, init) != 0) {
            free(init);
            break;
        }
#endif
        ++created;
    }

    if (created != thread_count) {
        atomic_store_explicit(ATOMIC_PTR(&pool->shutdown), true, memory_order_release);
        pool_wake_all(pool);
        join_workers(pool, created);         // only join the threads that really started
        destroy_pool_sync(pool);
        aligned_free_cl(pool);
        return NULL;
    }

    return pool;
}

Thrd* pool_make(void) {
    size_t detected = TOTAL_HARDWARE_THREADS;
#if defined(PCCORE_OS_LINUX)
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (sched_getaffinity(0, sizeof(allowed), &allowed) == 0) {
        const int allowed_count = CPU_COUNT(&allowed);
        if (allowed_count > 0 && (size_t)allowed_count < detected)
            detected = (size_t)allowed_count;
    } else {
        const long online = sysconf(_SC_NPROCESSORS_ONLN);
        if (online > 0 && (size_t)online < detected) detected = (size_t)online;
    }
#elif defined(PCCORE_OS_WINDOWS)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (info.dwNumberOfProcessors > 0 &&
        (size_t)info.dwNumberOfProcessors < detected) {
        detected = (size_t)info.dwNumberOfProcessors;
    }
#endif
    if (detected == 0) detected = 1;
    return pool_create(detected);
}

void pool_destroy(Thrd* pool) {
    if (!pool) return;

    atomic_store_explicit(ATOMIC_PTR(&pool->shutdown), true, memory_order_release);
    pool_wake_all(pool);                     // under wake_lock, so a sleeper can't miss it
    join_workers(pool, pool->thread_count);

    destroy_pool_sync(pool);
    aligned_free_cl(pool);
}

/* ------------------------------------------------------------------------- */
/* Public API: tasks / DAG                                                   */
/* ------------------------------------------------------------------------- */

[[nodiscard]] TaskHandle* pool_create_task_ex(
    Thrd* pool,
    TaskFunc func,
    void* arg,
    TaskPriority prio,
    CoreType target) {

    if (!pool || !func ||
        prio < TASK_PRIO_CRITICAL || prio >= TASK_PRIO_COUNT ||
        (target != CORE_TYPE_PCORE && target != CORE_TYPE_ECORE) ||
        atomic_load_explicit(ATOMIC_PTR(&pool->shutdown), memory_order_acquire)) {
        return NULL;
    }

    TaskHandle* task = arena_alloc_task(pool);   // returned with dependency_count == 1 (submission hold)
    task->func = func;
    task->arg = arg;
    task->priority = prio;
    task->target_core = target;
    return task;
}

/*
 * Contract: call this BEFORE either task is submitted (single-threaded graph build).
 * `dependent` runs only after `parent` finished. Edges added after submission
 * are rejected silently, because the task may already be running or recycled.
 */
void pool_add_dependency(TaskHandle* parent, TaskHandle* dependent) {
    if (!parent || !dependent || parent == dependent ||
        parent->pool == NULL || parent->pool != dependent->pool) {
        return;
    }

    if (atomic_load_explicit(ATOMIC_PTR(&parent->submit_state), memory_order_acquire) != 0 ||
        atomic_load_explicit(ATOMIC_PTR(&dependent->submit_state), memory_order_acquire) != 0) {
        return;
    }

    DependencyNode* node = malloc(sizeof(*node));
    if (!node) abort(); /* This API has no error return; fail rather than corrupt the DAG. */
    node->task = dependent;

    atomic_fetch_add_explicit(
        ATOMIC_PTR(&dependent->dependency_count), 1, memory_order_relaxed);

    DependencyNode* head = atomic_load_explicit(
        ATOMIC_PTR(&parent->dependents_head), memory_order_relaxed);
    do {
        node->next = head;
    } while (!atomic_compare_exchange_weak_explicit(
        ATOMIC_PTR(&parent->dependents_head), &head, node,
        memory_order_release, memory_order_relaxed));
}

void pool_submit_task(
    TaskHandle* task,
    pc_counter_t* counter,
    _Atomic(void*)* future_out) {

    if (!task || !task->pool) return;

    uint32_t expected_submit_state = 0;
    if (!atomic_compare_exchange_strong_explicit(
            ATOMIC_PTR(&task->submit_state), &expected_submit_state, 1,
            memory_order_acq_rel, memory_order_acquire)) {
        return; /* A task can be submitted only once. */
    }

    /* Keep the handle alive while we release the hold and possibly schedule it. */
    atomic_fetch_add_explicit(ATOMIC_PTR(&task->active), 1, memory_order_relaxed);
    task->counter = counter;
    task->feature_result = future_out;
    if (counter) {
        atomic_fetch_add_explicit(ATOMIC_PTR(counter), 1, memory_order_relaxed);
    }

    atomic_store_explicit(ATOMIC_PTR(&task->state), TASK_STATE_WAITING_DEP, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&task->submit_state), 2, memory_order_release);

    /* Release the submission hold. Whoever takes the count 1 -> 0 schedules the task, exactly once. */
    if (atomic_fetch_sub_explicit(
            ATOMIC_PTR(&task->dependency_count), 1, memory_order_acq_rel) == 1) {
        schedule_task(task);
    }
    task_release_reference(task);
}

/* ------------------------------------------------------------------------- */
/* Waiting                                                                   */
/* ------------------------------------------------------------------------- */

void pool_wait(pc_counter_t* counter) {
    if (!counter) return;

    for (;;) {
        const int32_t value = atomic_load_explicit(ATOMIC_PTR(counter), memory_order_acquire);
        if (value <= 0) return;
        sys_futex_wait(counter, value);      // woken when the counter reaches 0
    }
}

void pool_wait_active(Thrd* pool, pc_counter_t* counter) {
    if (!counter) return;

    /* Outside callers sleep on the futex; a worker must help instead of blocking itself. */
    if (!pool || !on_pool_worker(pool)) {
        pool_wait(counter);
        return;
    }

    const bool is_p_core =
        pool->info[g_worker_thread_id].core_type == CORE_TYPE_PCORE;
    uint32_t spin_count = 0;

    while (atomic_load_explicit(ATOMIC_PTR(counter), memory_order_acquire) > 0) {
        TaskHandle* task = find_task(pool, is_p_core);
        if (task) {
            execute_task_handle(task);
            spin_count = 0;
            continue;
        }

        if (++spin_count > 4096) {
            os_yield();
            spin_count = 0;
        } else {
            cpu_relax();
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Simple submit / fork / direct DAG                                         */
/* ------------------------------------------------------------------------- */

struct SimpleTaskPayload {
    void (*function)(void*);
    void* args;
};

static void* run_simple_task(TaskHandle* self, void* arg) {
    (void)self;
    struct SimpleTaskPayload* payload = arg;
    if (payload) {
        payload->function(payload->args);
        free(payload);
    }
    return NULL;
}

void pool_submit(Thrd* pool, void (*func)(void*), void* args, pc_counter_t* counter) {
    if (!pool || !func) return;

    struct SimpleTaskPayload* payload = malloc(sizeof(*payload));
    if (!payload) abort();
    payload->function = func;
    payload->args = args;

    TaskHandle* task = pool_create_task_ex(
        pool, run_simple_task, payload, TASK_PRIO_NORMAL, CORE_TYPE_PCORE);
    if (!task) {
        free(payload);
        return;
    }
    pool_submit_task(task, counter, NULL);
}

/*
 * Forks children of a running task. Children share the parent's counter and each
 * holds a reference on the parent, so the parent's handle outlives them.
 * Non-blocking: wait with pool_wait / pool_wait_active on the parent's counter.
 */
void pool_fork_active(
    TaskHandle* parent_task,
    TaskFunc sub_func,
    void** arg_array,
    size_t subtask_count) {

    if (!parent_task || !parent_task->pool || !sub_func ||
        atomic_load_explicit(ATOMIC_PTR(&parent_task->state), memory_order_acquire) != TASK_STATE_EXCUTING) {
        return;
    }

    for (size_t i = 0; i < subtask_count; ++i) {
        void* arg = arg_array ? arg_array[i] : NULL;
        TaskHandle* child = pool_create_task_ex(
            parent_task->pool, sub_func, arg,
            parent_task->priority, parent_task->target_core);
        if (!child) continue;

        child->parent = parent_task;
        atomic_fetch_add_explicit(ATOMIC_PTR(&parent_task->active), 1, memory_order_relaxed);
        pool_submit_task(child, parent_task->counter, NULL);
    }
}

static void task_vector_push(TaskHandle*** items, size_t* count, size_t* capacity, TaskHandle* task) {
    if (*count == *capacity) {
        const size_t next_capacity = (*capacity == 0) ? 64 : (*capacity * 2);
        if (next_capacity < *capacity || next_capacity > SIZE_MAX / sizeof(**items))
            abort();
        TaskHandle** next = realloc(*items, next_capacity * sizeof(**items));
        if (!next) abort();
        *items = next;
        *capacity = next_capacity;
    }
    (*items)[(*count)++] = task;
}

void pool_pcore_dag_excute_direct(TaskHandle* root_task, pc_counter_t* counter) {
    if (!root_task || !root_task->pool || !counter ||
        atomic_load_explicit(ATOMIC_PTR(&root_task->submit_state), memory_order_acquire) != 0) {
        return;
    }

    /*
     * Submit the whole reachable DAG in phases:
     *   1. claim + count every node (counter covers all descendants up front),
     *   2. publish them as submitted,
     *   3. release every submission hold; zero-dependency nodes start running.
     * This keeps the counter from reaching zero while descendants are still
     * being discovered.
     */
    TaskHandle** stack = NULL;
    TaskHandle** claimed = NULL;
    size_t stack_count = 0, stack_capacity = 0;
    size_t claimed_count = 0, claimed_capacity = 0;
    Thrd* pool = root_task->pool;

    task_vector_push(&stack, &stack_count, &stack_capacity, root_task);
    while (stack_count != 0) {
        TaskHandle* task = stack[--stack_count];
        if (!task || task->pool != pool) continue;

        uint32_t expected = 0;
        if (!atomic_compare_exchange_strong_explicit(
                ATOMIC_PTR(&task->submit_state), &expected, 1,
                memory_order_acq_rel, memory_order_acquire)) {
            continue; /* Already visited, or submitted through another API. */
        }

        atomic_fetch_add_explicit(ATOMIC_PTR(&task->active), 1, memory_order_relaxed);
        task->counter = counter;
        task->feature_result = NULL;
        atomic_fetch_add_explicit(ATOMIC_PTR(counter), 1, memory_order_relaxed);
        task_vector_push(&claimed, &claimed_count, &claimed_capacity, task);

        DependencyNode* node = atomic_load_explicit(
            ATOMIC_PTR(&task->dependents_head), memory_order_acquire);
        while (node) {
            task_vector_push(&stack, &stack_count, &stack_capacity, node->task);
            node = node->next;
        }
    }

    for (size_t i = 0; i < claimed_count; ++i) {
        atomic_store_explicit(ATOMIC_PTR(&claimed[i]->state), TASK_STATE_WAITING_DEP, memory_order_relaxed);
        atomic_store_explicit(ATOMIC_PTR(&claimed[i]->submit_state), 2, memory_order_release);
    }
    for (size_t i = 0; i < claimed_count; ++i) {
        if (atomic_fetch_sub_explicit(
                ATOMIC_PTR(&claimed[i]->dependency_count), 1, memory_order_acq_rel) == 1) {
            schedule_task(claimed[i]);
        }
    }
    for (size_t i = 0; i < claimed_count; ++i) {
        task_release_reference(claimed[i]);
    }

    free(stack);
    free(claimed);
    pool_wait_active(pool, counter);
}

/* ------------------------------------------------------------------------- */
/* FFI bridge                                                                */
/* ------------------------------------------------------------------------- */

[[nodiscard]] FFMContext* ffi_create_context(void* java_in, void* java_out, size_t size) {
    FFMContext* ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->java_in_buffer = java_in;
    ctx->java_out_buffer = java_out;
    ctx->data_size = size;
    ctx->flags = 0;

    /* Own a real cache-line-aligned copy: every byte is usable. */
    if (size != 0) {
        ctx->pcore_aligned_ptr = aligned_alloc_cl(size);
        if (!ctx->pcore_aligned_ptr) {
            free(ctx);
            return NULL;
        }
        if (java_in) memcpy(ctx->pcore_aligned_ptr, java_in, size);
        else memset(ctx->pcore_aligned_ptr, 0, size);
    }

    return ctx;
}

void ffi_bridge_free_context(FFMContext* ctx) {
    if (!ctx) return;
    aligned_free_cl(ctx->pcore_aligned_ptr);
    free(ctx);
}

void pool_submit_ffi_dag_pipeline(
    Thrd* pool,
    FFMContext* ffi_ctx,
    TaskFunc pcore_compute_func,
    pc_counter_t* counter) {

    if (!pool || !ffi_ctx || !pcore_compute_func) return;

    TaskHandle* task = pool_create_task_ex(
        pool, pcore_compute_func, ffi_ctx, TASK_PRIO_HIGH, CORE_TYPE_PCORE);
    if (!task) return;
    pool_submit_task(task, counter, NULL);
}

/* ------------------------------------------------------------------------- */
/* PDE (parallel data execution): caller side                                */
/* ------------------------------------------------------------------------- */

void pool_pde_parallel(
    Thrd* pool,
    size_t total_element,
    ParallelFunc func,
    void* user_data) {

    if (!pool || !func || total_element == 0 ||
        atomic_load_explicit(ATOMIC_PTR(&pool->shutdown), memory_order_acquire)) {
        return;
    }

    /* Calling the dispatcher from one of its own workers would self-deadlock. */
    if (on_pool_worker(pool)) {
        ParallelRange range = {
            .start_idx = 0,
            .end_idx = total_element,
            .thread_id = g_worker_thread_id,
            .user_data = user_data,
            .pool = pool
        };
        ++g_serial_pde_depth;
        func(&range); /* Nested PDE falls back to a serial range on this worker. */
        --g_serial_pde_depth;
        return;
    }

    /* call_lock serialises whole rounds so two callers cannot overlap. */
    native_mutex_lock(&pool->pde.call_lock);

    const size_t tc = pool->thread_count;
    const size_t chunk = 1 + (total_element - 1) / tc;

    pool->pde.func = func;
    pool->pde.user_data = user_data;
    pool->pde.total_elements = total_element;
    pool->pde.barrier.total_thread = 1 + (total_element - 1) / chunk;
    atomic_store_explicit(ATOMIC_PTR(&pool->pde.barrier.in_barrier), 0, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&pool->pde.barrier.out_barrier), 0, memory_order_relaxed);
    atomic_store_explicit(ATOMIC_PTR(&pool->pde.pending_workers), tc, memory_order_release);
    atomic_fetch_add_explicit(ATOMIC_PTR(&pool->pde.generation), 1, memory_order_release);

    pool_wake_all(pool);

    native_mutex_lock(&pool->pde.dispath_lock);
    while (atomic_load_explicit(ATOMIC_PTR(&pool->pde.pending_workers), memory_order_acquire) != 0) {
        native_cond_wait(&pool->pde.dispath_signals, &pool->pde.dispath_lock);
    }
    native_mutex_unlock(&pool->pde.dispath_lock);

    native_mutex_unlock(&pool->pde.call_lock);
}

// Reusable barrier. futex_val is a generation counter, so a fast thread entering the
// next barrier can't confuse a slow waiter from the previous one.
void pool_pde_barrier(ParallelRange* range) {
    if (!range || !range->pool || g_serial_pde_depth != 0) return;

    Barrier* barrier = &range->pool->pde.barrier;
    const size_t participants = barrier->total_thread;
    if (participants <= 1 || range->thread_id >= participants) return;

    const int32_t generation = atomic_load_explicit(
        ATOMIC_PTR(&barrier->futex_val), memory_order_acquire);

    if (atomic_fetch_add_explicit(
            ATOMIC_PTR(&barrier->in_barrier), 1, memory_order_acq_rel) + 1 == participants) {
        atomic_store_explicit(ATOMIC_PTR(&barrier->in_barrier), 0, memory_order_relaxed);
        atomic_fetch_add_explicit(ATOMIC_PTR(&barrier->futex_val), 1, memory_order_release);
        sys_futex_wake((pc_counter_t*)&barrier->futex_val, INT32_MAX);
        return;
    }

    while (atomic_load_explicit(
            ATOMIC_PTR(&barrier->futex_val), memory_order_acquire) == generation) {
        sys_futex_wait((pc_counter_t*)&barrier->futex_val, generation);
    }
}

/* ------------------------------------------------------------------------- */
/* Manual pop / execute                                                      */
/* ------------------------------------------------------------------------- */

bool pool_pop_task(Thrd* pool, TaskHandle** out_task) {
    if (!pool || !out_task) return false;
    *out_task = find_task(pool, true);
    return *out_task != NULL;
}

void pool_execute_task(TaskHandle* task) {
    execute_task_handle(task);
}