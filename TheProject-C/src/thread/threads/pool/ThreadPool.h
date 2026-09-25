#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

// ============================================================================
// OS DETECT & NATIVE HEADERS INCLUSION
// ============================================================================
#if defined(_WIN32) || defined(_WIN64)
    #define PCCORE_OS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <processthreadsapi.h>
    #include <synchapi.h>
#elif defined(__linux__)
    #define PCCORE_OS_LINUX
    #define _GNU_SOURCE
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <sys/time.h>
    #include <sys/futex.h>
    #include <linux/futex.h>
#else
    #error "Platform không được hỗ trợ! Chỉ hỗ trợ Native Windows và Native Linux."
#endif

// ============================================================================
// COMPILER & HARDWARE CONSTANTS
// ============================================================================
#if defined(__GNUC__) || defined(__clang__)
    #define PCCORE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define PCCORE_INLINE __forceinline
#else
    #define PCCORE_INLINE inline
#endif

constexpr size_t CACHE_LINE = 64;
constexpr size_t TASK_QUEUE_CAPACITY = 8192;
constexpr size_t TOTAL_HARDWARE_THREADS = 24;
constexpr size_t TOTAL_HARDWARE_CORES = 16;

// ============================================================================
// NATIVE OS PRIMITIVES ABSTRACTION
// ============================================================================
#if defined(PCCORE_OS_WINDOWS)
    typedef HANDLE                  native_thread_t;
    typedef CRITICAL_SECTION        native_mutex_t;
    typedef CONDITION_VARIABLE      native_cond_t;
    typedef DWORD                   native_thread_id_t;
#elif defined(PCCORE_OS_LINUX)
    typedef pthread_t               native_thread_t;
    typedef pthread_mutex_t         native_mutex_t;
    typedef pthread_cond_t          native_cond_t;
    typedef pid_t                   native_thread_id_t;
#endif

// Forward Declarations
typedef struct Thrd Thrd;
typedef struct TaskHandle TaskHandle;

// ============================================================================
// ENUMS & TYPEDEFS
// ============================================================================
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

typedef struct DependencyNode
{
     TaskHandle* task;
     struct DependencyNode* next;
} DependencyNode;

typedef void* (*TaskFunc)(TaskHandle* self, void* arg);

struct alignas(CACHE_LINE) TaskHandle
{
     TaskFunc func;
     void* arg;

     _Atomic(size_t)* counter;
     _Atomic(void*)* feature_result;

     alignas(CACHE_LINE) _Atomic(int32_t) dependency_count;
     alignas(CACHE_LINE) _Atomic(DependencyNode*) dependents_head;
     _Atomic(TaskState) state;

     TaskHandle* parent;
     _Atomic(size_t) active;

     TaskPriority priority;
     CoreType target_core;
     Thrd* pool;
};

typedef struct
{
     void (*function)(void* args);
     void* args;
     _Atomic(size_t)* counter;
} Task;

typedef struct
{
     _Atomic size_t squence;
     TaskHandle* task;
} QueueCell;

typedef struct alignas(CACHE_LINE)
{
     void* java_in_buffer;
     void* java_out_buffer;
     void* pcore_aligned_ptr;
     size_t data_size;
     uint32_t flags;
} FFIBridgeContext;

typedef struct alignas(CACHE_LINE)
{
     QueueCell ring[TASK_QUEUE_CAPACITY];
     _Atomic size_t head;
     _Atomic size_t tail;
} MPMCQueue;

typedef struct
{
     _Atomic(int32_t) futex_val; // Dùng trực tiếp cho Linux Futex / Windows WaitOnAddress
     _Atomic(size_t) in_barrier;
     _Atomic(size_t) out_barrier;
     size_t total_thread;
} Barrier;

typedef struct
{
     size_t start_idx;
     size_t end_idx;
     size_t thread_id;
     void* user_data;
     Thrd* pool;
} ParallelRange;

typedef void (*ParallelFunc)(ParallelRange* range);

// ============================================================================
// MAIN THREAD POOL STRUCTURE (NATIVE OS BASED)
// ============================================================================
struct Thrd
{
     native_thread_t threads[TOTAL_HARDWARE_THREADS];
     ThreadInfo info[TOTAL_HARDWARE_THREADS];
     size_t thread_count;
     _Atomic bool shutdown;

     MPMCQueue p_queues[TASK_PRIO_COUNT];
     MPMCQueue e_queues[TASK_PRIO_COUNT];

     struct
     {
          Task task[TASK_QUEUE_CAPACITY];
          _Atomic size_t head;
          _Atomic size_t tail;
          native_cond_t signals;
          native_mutex_t lock;
     } queue;

     struct
     {
          ParallelFunc func;
          void* user_data;
          size_t total_elements;
          _Atomic(size_t) pending_workers;
          Barrier barrier;
          native_cond_t dispath_signals;
          native_mutex_t dispath_lock;
          _Atomic bool pde_mode;
     } pde;

     native_cond_t wake_signals;
     native_mutex_t wake_lock;

     struct alignas(CACHE_LINE)
     {
          TaskHandle pool_arena[TASK_QUEUE_CAPACITY * 2];
          _Atomic(size_t) arena_idx;
     } memory_arena;
};

// ============================================================================
// NATIVE FUTEX / WAIT WRAPPERS (ZERO OVERHEAD SYNCHRONIZATION)
// ============================================================================
PCCORE_INLINE void sys_futex_wait(_Atomic(int32_t)* addr, int32_t val) {
#if defined(PCCORE_OS_LINUX)
    syscall(SYS_futex, (int32_t*)addr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
#elif defined(PCCORE_OS_WINDOWS)
    int32_t expected = val;
    WaitOnAddress((volatile void*)addr, &expected, sizeof(int32_t), INFINITE);
#endif
}

PCCORE_INLINE void sys_futex_wake(_Atomic(int32_t)* addr, int32_t count) {
#if defined(PCCORE_OS_LINUX)
    syscall(SYS_futex, (int32_t*)addr, FUTEX_WAKE_PRIVATE, count, NULL, NULL, 0);
#elif defined(PCCORE_OS_WINDOWS)
    if (count == 1) {
        WakeByAddressSingle((void*)addr);
    } else {
        WakeByAddressAll((void*)addr);
    }
#endif
}

// ============================================================================
// PUBLIC THREAD POOL API
// ============================================================================
[[nodiscard]] Thrd* pool_create(size_t thread_count);
Thrd* pool_make(void);

void pool_destroy(Thrd* pool);

void pool_bind_thread_affinity(size_t thread_id);

void pool_submit(Thrd* pool, void(*func)(void*), void* args, _Atomic(size_t)* counter);
void pool_wait(_Atomic(size_t)* counter);

[[nodiscard]] TaskHandle* pool_create_task_ex(Thrd* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType target);
void pool_add_dependency(TaskHandle* parent, TaskHandle* dependent);
void pool_submit_task(TaskHandle* task, _Atomic(size_t)* counter, _Atomic(size_t)* future_out);

void pool_wait_active(Thrd* pool, _Atomic(size_t)* counter);

void pool_fork_active(TaskHandle* parent_task, TaskFunc sub_func, void** arg_array, size_t subtask_count);

void pool_pcore_dag_excute_direct(TaskHandle* root_task, _Atomic(size_t)* counter);

[[nodiscard]] FFIBridgeContext* ffi_create_context(void* java_in, void* java_out, size_t size);
void ffi_bridge_free_context(FFIBridgeContext* ctx);

void pool_submit_ffi_dag_pipeline(Thrd* pool, FFIBridgeContext* ffi_ctx, TaskFunc pcore_compute_func, _Atomic(size_t)* counter);

void pool_pde_parallel(Thrd* pool, size_t total_element, ParallelFunc func, void* user_data);
void pool_pde_barrier(ParallelRange* range);
