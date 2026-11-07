//
// Created by BaoZhuang in 31/10/2026
//
#pragma once

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined(PCCORE_OS_WINDOWS)
#include <malloc.h>
#endif

#if defined(__WIN32__) || defined(__linux__)
#include <immintrin.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define PCCORE_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <synchapi.h>
#elif defined(__linux__)
#define PCCORE_OS_LINUX
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/futex.h>
#include <linux/futex.h>
#else
#error "Platform not supported."
#endif

#if defined(__GNUC__) || defined(__clang__)
#define INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define INLINE inline __forceinline
#else
#define INLINE inline
#endif

#define CL 64 // CACHE LINE
#define TQ_CAPACITY 8192 //TASK QUEUE
#define TOTAL_HARDWARE_THREADS 24
#define TOTAL_HARDWARE_CORES 16


#if defined(__CLANG_STDATOMIC_H)
#define ATOMIC_PTR(p) (p)
#else
#define ATOMIC_PTR(p) ((__typeof__((void)0, *(p))*)(p))
#endif

typedef _Atomic(int32_t) pc_counter_t;

#if defined(PCCORE_OS_WINDOWS)
typedef HANDLE native_thread_t;
typedef CRITICAL_SECTION native_mutex_t;
typedef CONDITION_VARIABLE native_cond_t;
typedef DWORD native_thread_id_t;
#elif defined(PCCORE_OS_LINUX)
typedef pthread_t native_thread_t;
typedef pthread_mutex_t native_mutex_t;
typedef pthread_cond_t native_cond_t;
typedef pid_t native_thread_id_t;
#endif

typedef struct Thrd Thrd;
typedef struct TaskHandle TaskHandle;
typedef struct DependencyNode DependencyNode;
typedef struct FFMContext FFMContext;
typedef struct ParallelRange ParallelRange;
typedef struct QueueCell QueueCell;
typedef struct Task Task;
typedef struct MPMCQueue MPMCQueue;
typedef struct Barrier Barrier;
typedef struct memory_arena memory_arena;
typedef struct pde pde;
typedef void (*ParallelFunc)(ParallelRange* range);
typedef void* (*TaskFunc)(TaskHandle* self, void* arg);

typedef enum
{
    THREAD_0 = 0,
    THREAD_1 = 1,
    THREAD_2 = 2,
    THREAD_3 = 3,
    THREAD_4 = 4,
    THREAD_5 = 5,
    THREAD_6 = 6,
    THREAD_7 = 7,
    THREAD_8 = 8,
    THREAD_9 = 9,
    THREAD_10 = 10,
    THREAD_11 = 11,
    THREAD_12 = 12,
    THREAD_13 = 13,
    THREAD_14 = 14,
    THREAD_15 = 15,
    THREAD_16 = 16,
    THREAD_17 = 17,
    THREAD_18 = 18,
    THREAD_19 = 19,
    THREAD_20 = 20,
    THREAD_21 = 21,
    THREAD_22 = 22,
    THREAD_23 = 23
} Threads;

typedef enum
{
    CORE_TYPE_PCORE = 0,
    CORE_TYPE_ECORE = 1
} CoreType;

typedef enum
{
    TASK_PRIO_CRITICAL = 0,
    TASK_PRIO_HIGH = 1,
    TASK_PRIO_NORMAL = 2,
    TASK_PRIO_LOW = 3,
    TASK_PRIO_COUNT = 4
} TaskPriority;

typedef enum
{
    TASK_STATE_FREE = 0,
    TASK_STATE_WAITING_DEP,
    TASK_STATE_READY,
    TASK_STATE_EXCUTING,
    TASK_STATE_FINISHED
} TaskState;

typedef struct
{
    size_t thread_id;
    size_t core_id;
    CoreType core_type;
    bool is_smt_thread;
    native_thread_id_t os_tid;
} ThreadInfo;

struct DependencyNode{
    TaskHandle* task;
    struct DependencyNode* next;
};

struct TaskHandle {
    alignas(CL) _Atomic(int32_t) dependency_count;
    alignas(CL) _Atomic(DependencyNode*) dependents_head;

    TaskFunc func;
    void* arg;
    pc_counter_t* counter;
    _Atomic(void*)* feature_result;
    struct TaskHandle *parent;
    _Atomic(size_t) active;
    _Atomic(uint32_t) submit_state;
    Thrd* pool;

    _Atomic(TaskState) state;
    TaskPriority priority;
    CoreType target_core;
} __attribute__((aligned(CL)));

struct Task {
    void (*function)(void* args);
    void* args;
    pc_counter_t* counter;
};

struct QueueCell {
    _Atomic size_t squence;
    TaskHandle* task;
};

struct FFMContext {
    void* java_in_buffer;
    void* java_out_buffer;
    void* pcore_aligned_ptr;
    size_t data_size;
    uint32_t flags;
};

struct MPMCQueue {
    QueueCell ring[TQ_CAPACITY];
    _Atomic size_t head;
    _Atomic size_t tail;
} __attribute__((aligned(CL)));

struct Barrier {
    _Atomic(int32_t) futex_val;
    _Atomic(size_t) in_barrier;
    _Atomic(size_t) out_barrier;
    size_t total_thread;
};

struct ParallelRange {
    size_t start_idx;
    size_t end_idx;
    size_t thread_id;
    void* user_data;
    Thrd* pool;
};

struct memory_arena {
    TaskHandle pool_arena[TQ_CAPACITY * 2];
    _Atomic(size_t) arena_idx;
} __attribute__((aligned(CL)));

struct pde {
    ParallelFunc func;
    void* user_data;
    size_t total_elements;
    _Atomic(size_t) pending_workers;
    _Atomic(uint32_t) generation;
    Barrier barrier;

    native_cond_t dispath_signals;
    native_mutex_t dispath_lock;
    native_mutex_t call_lock;
};

struct Thrd {
    native_thread_t threads[TOTAL_HARDWARE_THREADS];
    ThreadInfo info[TOTAL_HARDWARE_THREADS];
    size_t thread_count;
    _Atomic bool shutdown;

    MPMCQueue p_queue[TASK_PRIO_COUNT];
    MPMCQueue e_queue[TASK_PRIO_COUNT];

    native_cond_t wake_signals;
    native_mutex_t wake_lock;

    memory_arena memory_arena;
    pde pde;
};

static INLINE void sys_futex_wait(pc_counter_t* addr, int32_t val) {
#if defined(PCCORE_OS_LINUX)
    syscall(SYS_futex, (int32_t*)addr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);   // was YS_futex
#elif defined(PCCORE_OS_WINDOWS)
    int32_t expected = val;
    WaitOnAddress((volatile void*)addr, &expected, sizeof(int32_t), INFINITE);
#endif
}

static INLINE void sys_futex_wake(pc_counter_t* addr, int32_t count) {
#if defined(PCCORE_OS_LINUX)
    syscall(SYS_futex, (int32_t*)addr, FUTEX_WAKE_PRIVATE, count, NULL, NULL, 0);
#elif defined(PCCORE_OS_WINDOWS)
    if (count == 1) WakeByAddressSingle((void*)addr);
    else            WakeByAddressAll((void*)addr);
#endif
}

[[nodiscard]] Thrd* pool_create(size_t thread_count);
Thrd* pool_make(void);

void pool_destroy(Thrd* pool);

void pool_bind_thread_affinity(size_t core_id);

void pool_submit(Thrd* pool, void(*func)(void*), void* args, pc_counter_t* counter);
void pool_wait(pc_counter_t* counter);

[[nodiscard]] TaskHandle* pool_create_task_ex(Thrd* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType target);
void pool_add_dependency(TaskHandle* parent, TaskHandle* dependent);
void pool_submit_task(TaskHandle* task, pc_counter_t* counter, _Atomic(void*)* future_out);
void pool_wait_active(Thrd* pool, pc_counter_t* counter);

void pool_fork_active(TaskHandle* parent_task, TaskFunc sub_func, void** arg_array, size_t subtask_count);

void pool_pcore_dag_excute_direct(TaskHandle* root_task, pc_counter_t* counter);

[[nodiscard]] FFMContext* ffi_create_context(void* java_in, void* java_out, size_t size);
void ffi_bridge_free_context(FFMContext* ctx);

void pool_submit_ffi_dag_pipeline(Thrd* pool, FFMContext* ffi_ctx, TaskFunc pcore_compute_func, pc_counter_t* counter);

void pool_pde_parallel(Thrd* pool, size_t total_element, ParallelFunc func, void* user_data);
void pool_pde_barrier(ParallelRange* range);

bool pool_pop_task(Thrd* pool, TaskHandle** out_handle);
void pool_execute_task(TaskHandle* task);
