#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>

// Kế thừa các kiểu dữ liệu cốt lõi: Thrd, TaskPriority, CoreType, TaskFunc, TaskHandle...
#include <_mingw_off_t.h>

#include "ThreadPool.h"


// Include các OS Native Header cho Thread API
#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <processthreadsapi.h>
#else
    #include <pthread.h>
    #include <sys/types.h>
#endif

constexpr size_t PRIV_QUEUE_CAPACITY = 8192;
constexpr size_t CACHE_LINE_SIZE = 64;

// Struct gói gọn 1 Task Item trong Local Queue
// Kế thừa TaskPriority và CoreType từ thread_pool.h
typedef struct TaskItem
{
    TaskFunc func;
    void* user_data;
    TaskPriority priority;
    CoreType target_core;
    _Atomic(size_t)* counter;
    _Atomic(void*)* future_result;
} TaskItem;

// Đổi tên thành PrivatizedQueueCell để tránh trùng tên với QueueCell của thread_pool.h
typedef struct PrivatizedQueueCell
{
    _Atomic size_t sequence;
    TaskItem data;
} PrivatizedQueueCell;

typedef struct PrivatizedTaskQueue
{
    PrivatizedQueueCell buffer[PRIV_QUEUE_CAPACITY];
    size_t buffer_mask;

    alignas(CACHE_LINE_SIZE) _Atomic size_t front;
    alignas(CACHE_LINE_SIZE) _Atomic size_t back;
    alignas(CACHE_LINE_SIZE) _Atomic size_t num_items;

    CoreType bound_core;
} PrivatizedTaskQueue;

// Struct mảng động cho Chase-Lev Deque
typedef struct DynamicTaskArray
{
    size_t capacity;
    size_t mask;
    _Atomic(TaskItem) buffer[];
} DynamicTaskArray;

// Dynamic Lock-Free Work-Stealing Deque (Chase-Lev)
typedef struct ChaseLevDeque
{
    alignas(CACHE_LINE_SIZE) _Atomic size_t top;
    alignas(CACHE_LINE_SIZE) _Atomic size_t bottom;
    alignas(CACHE_LINE_SIZE) _Atomic(DynamicTaskArray*) array;
} ChaseLevDeque;

// Native Linux io_uring Async Engine
typedef struct IoUringEngine
{
    int ring_fd;
    void* sq_ring;
    void* cq_ring;
    void* sqes;
    void* cqes;
    uint32_t sq_ring_mask;
    uint32_t cq_ring_mask;
    bool enabled;
} IoUringEngine;

// Worker Struct đóng gói theo Native OS Primitives
typedef struct Worker
{
#if defined(_WIN32) || defined(_WIN64)
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cv;
#else
    pthread_mutex_t lock;
    pthread_cond_t cv;
#endif

    PrivatizedTaskQueue queue;
    TaskItem cache_task;
    bool has_cache;
    bool exit;
    size_t last_victim;

    ChaseLevDeque* dynamic_deque;
    IoUringEngine io_uring;
} Worker;

// Privatized Thread Pool
typedef struct PrivatizedThreadPool
{
    Thrd* base_pool;        // Kế thừa / Wrapper lớp Base Engine (Thrd từ thread_pool.h)
    size_t num_workers;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE* threads;        // Dùng HANDLE từ <processthreadsapi.h>
#else
    pthread_t* threads;
#endif

    Worker* workers;
    _Atomic size_t total_tasks;
    _Atomic bool stop;
    uint64_t rng_seed;
} PrivatizedThreadPool;

// =============================================================================
// PUBLIC APIS
// =============================================================================

[[nodiscard]] PrivatizedThreadPool* privatized_pool_create(size_t num_workers);

void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core);

void privatized_pool_submit_ex(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core, _Atomic(size_t)* counter, _Atomic(void*)* future_out);

void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count);

size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch);

bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data);

void privatized_pool_destroy(PrivatizedThreadPool* pool);