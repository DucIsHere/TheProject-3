#pragma once

#include "ThreadPool.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <_mingw_off_t.h>

constexpr size_t PRIV_QUEUE_CAPACITY = 8192;

typedef struct TaskItem TaskItem;
typedef struct QC QC;
typedef struct PrivatizedTaskQueue PrivatizedTaskQueue;
typedef struct DynamicTaskArray DynamicTaskArray;
typedef struct IoUringEngine IoUringEngine;
typedef struct Worker Worker;
typedef struct ChaseLevDeque ChaseLevDeque;
typedef struct PrivatizedThreadPool PrivatizedThreadPool;

struct TaskItem {
    TaskFunc func;
    void* user_data;
    TaskPriority priority;
    CoreType target_core;
    _Atomic(size_t)* counter;
    _Atomic(void*)* future_result;
};

struct QC {
    _Atomic size_t squence;
    TaskItem datd;
};

struct PrivatizedTaskQueue {
    QC buffers[PRIV_QUEUE_CAPACITY];
    size_t buffer_mask;

    alignas(64) _Atomic(size_t) front;
    alignas(64) _Atomic(size_t) back;
    alignas(64) _Atomic(size_t) num_items;
};

struct DynamicTaskArray {
    size_t capacity;
    size_t mask;
    _Atomic(TaskItem) buffer[];
};

struct ChaseLevDeque {
    alignas(64) _Atomic size_t top;
    alignas(64) _Atomic size_t bottom;
    alignas(64) _Atomic(DynamicTaskArray*) array;
};

struct IoUringEngine {
    int ring_fd;
    void* sq_ring;
    void* cq_ring;
    void* sqes;
    void* cqes;
    uint32_t sq_ring_mask;
    uint32_t cq_ring_mask;
    bool enabled;
};

struct Worker {
    native_cond_t cv;
    native_mutex_t lock;
    PrivatizedTaskQueue queue;
    TaskItem cache_task;
    bool has_cache;
    bool exit;
    size_t last_victim;

    struct ChaseLevDeque* dynamic_deque;
    IoUringEngine io_uring;
};

struct PrivatizedThreadPool {
    Thrd* base_pool;
    size_t num_workers;

#if defined(WIN32) || defined(WIN64)
    void** threads;
#else
    pthread_t* threads;
#endif

    Worker* workers;
    _Atomic size_t total_tasks;
    _Atomic bool stop;
    uint64_t rng_seed;
};

// Khởi tạo PrivatizedThreadPool (tự động tạo Base Engine bên dưới)
[[nodiscard]] PrivatizedThreadPool* privatized_pool_create(size_t num_workers);

// Đẩy Task vào Pool (Fast-path qua Local Deque, Fallback qua Base Pool)
void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core);

// Submit Task có hỗ trợ Counter đồng bộ và Future Result
void privatized_pool_submit_ex(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core, _Atomic(size_t)* counter, _Atomic(void*)* future_out);

// SIMD Task Batching ($O(1)$ Lock-Free Push nhiều Task cùng lúc)
void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count);

// Dequeue theo Batch ra L1 Cache để xử lý Vector SIMD (AVX2 / AVX-512)
size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch);

// Submit Async Kernel I/O (Read/Write) trực tiếp qua io_uring
bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data);

// Dọn dẹp và hủy PrivatizedThreadPool
void privatized_pool_destroy(PrivatizedThreadPool* pool);
