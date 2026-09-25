#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include <_mingw_off_t.h>

// Kiểm tra OS để include thư viện io_uring riêng cho Linux
#if defined(__linux__)
#include <liburing.h>
#endif

#include "ThreadPool.h"

#define PRIV_QUEUE_CAPACITY 1024
#define CACHE_LINE_SIZE 64

// =============================================================================
// CORE TASK & QUEUE STRUCTURES
// =============================================================================

typedef struct {
    TaskFunc func;
    void* user_data;
    TaskPriority priority;
    CoreType target_core;
} TaskItem;

typedef struct {
    _Atomic size_t squence;
    TaskItem data;
} LocalQueueCell;

typedef struct {
    LocalQueueCell buffer[PRIV_QUEUE_CAPACITY];
    size_t buffer_mask;

#if defined(_MSC_VER)
    __declspec(align(CACHE_LINE_SIZE)) _Atomic size_t front;
    __declspec(align(CACHE_LINE_SIZE)) _Atomic size_t back;
    __declspec(align(CACHE_LINE_SIZE)) _Atomic size_t num_items;
#else
    _Atomic size_t front __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic size_t back __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic size_t num_items __attribute__((aligned(CACHE_LINE_SIZE)));
#endif

    CoreType bound_core;
} PrivatizedTaskQueue;

// =============================================================================
// CHASE-LEV LOCK-FREE DEQUE
// =============================================================================

typedef struct DynamicTaskArray {
    size_t capacity;
    size_t mask;
    _Atomic(TaskItem) buffer[];
} DynamicTaskArray;

typedef struct ChaseLevDeque {
#if defined(_MSC_VER)
    __declspec(align(CACHE_LINE_SIZE)) _Atomic size_t top;
    __declspec(align(CACHE_LINE_SIZE)) _Atomic size_t bottom;
    __declspec(align(CACHE_LINE_SIZE)) _Atomic(DynamicTaskArray*) array;
#else
    _Atomic size_t top __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic size_t bottom __attribute__((aligned(CACHE_LINE_SIZE)));
    _Atomic(DynamicTaskArray*) array __attribute__((aligned(CACHE_LINE_SIZE)));
#endif
} ChaseLevDeque;

// =============================================================================
// KERNEL IO_URING ENGINE (LINUX ONLY)
// =============================================================================

struct io_uring_sqe;
struct io_uring_cqe;

typedef struct IoUringEngine {
    int ring_fd;
    void* sq_ring;
    void* cq_ring;
    struct io_uring_sqe* sqes;
    struct io_uring_cqe* cqes;
    uint32_t sq_ring_mask;
    uint32_t cq_ring_mask;
    bool enabled;
} IoUringEngine;

// =============================================================================
// WORKER & PRIVATIZED THREAD POOL ENGINE
// =============================================================================

typedef struct Worker {
    native_cond_t cv;
    native_mutex_t lock;
    PrivatizedTaskQueue queue;
    TaskItem cache_task;
    bool has_cache;
    bool exit;
    size_t last_victim;

    ChaseLevDeque* dynamic_deque;
    IoUringEngine io_uring;
} Worker;

typedef struct PrivatizedThreadPool {
    Thrd* base_pool;             // Global Fallback Queue từ thread_pool.h
    size_t num_workers;
    native_thread_t* threads;
    Worker* workers;
    _Atomic size_t total_tasks;
    _Atomic bool stop;
    uint64_t rng_seed;
} PrivatizedThreadPool;

// =============================================================================
// PUBLIC APIS
// =============================================================================

// Tạo PrivatizedThreadPool
PrivatizedThreadPool* privatized_pool_create(size_t num_workers);

// Đẩy Task vào Pool (Fast-path qua Local Queue/Cache Slot, Fallback qua Main Queue)
void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core);

// Hủy Pool
void privatized_pool_destroy(PrivatizedThreadPool* pool);

// SIMD Task Batching ($O(1)$ Lock-Free Push nhiều Task)
void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count);

// Dequeue theo Batch ra L1 Cache để xử lý Vector SIMD (AVX2 / AVX-512)
size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch);

// Submit Async I/O (Read/Write) thẳng xuống Kernel qua io_uring (Linux Native)
bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data);