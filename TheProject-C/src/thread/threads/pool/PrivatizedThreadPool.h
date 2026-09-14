#pragma once

#pragma GCC optimize("03", "unroll-loops", "omit-frame-pointer")
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include <liburing.h>

#include "ThreadPool.h"

constexpr size_t PRIV_QUEUE_CAPACITY = 1024;
constexpr size_t CACHE_LINE_SIZE = 64;

typedef struct 
{
     TaskFunc func;
     void* user_data;
     TaskPriority priority;
     CoreType target_core;
} TaskItem;

typedef struct 
{
     _Atomic size_t sequence;
     TaskItem data;
} QueueCell;

typedef struct 
{
     QueueCell buffer[PRIV_QUEUE_CAPACITY];
     size_t buffer_mask;

     #pragma pack(push, 8)
     alignas(CACHE_LINE_SIZE) _Atomic size_t front;
     alignas(CACHE_LINE_SIZE) _Atomic size_t back;
     alignas(CACHE_LINE_SIZE) _Atomic size_t num_items;
     #pragma pack(pop)

     CoreType bound_core;
} PrivatizedTaskQueue;

typedef struct Worker
{
     cnd_t cv;
     mtx_t lock;
     PrivatizedTaskQueue queue;
     TaskItem cache_task;
     bool has_cache;
     bool exit;
     size_t last_victim;

     ChaseLevDeque* dynamic_deque;
     IoUringEngine io_uring;
} Worker;

typedef struct DynamicTaskArray
{
     size_t capacity;
     size_t mask;
     _Atomic(TaskItem) buffer[];
} DynamicTaskArray;

typedef struct ChaseLevDequeue
{
     alignas(CACHE_LINE_SIZE) _Atomic size_t top;
     alignas(CACHE_LINE_SIZE) _Atomic size_t bottom;
     alignas(CACHE_LINE_SIZE) _Atomic(DynamicTaskArray*) array;
} ChaseLevDeque;

struct io_uring_sqe;
struct io_uring_cqe;

typedef struct IoUringEngine
{
     int ring_fd;
    void* sq_ring;
    void* cq_ring;
    struct io_uring_sqe* sqes;
    struct io_uring_cqe* cqes;
    uint32_t sq_ring_mask;
    uint32_t cq_ring_mask;
    bool enabled;
} IoUringEngine;

typedef struct PrivatizedThreadPool {
    Thrd* base_pool;        // Global Fallback Queue từ ThreadPool.h
    size_t num_workers;
    thrd_t* threads;
    Worker* workers;
    _Atomic size_t total_tasks;
    _Atomic bool stop;
    uint64_t rng_seed;
} PrivatizedThreadPool;

// Tạo PrivatizedThreadPool
[[nodiscard]] PrivatizedThreadPool* privatized_pool_create(size_t num_workers);

// Đẩy Task vào Pool (Fast-path qua Local Queue/Cache Slot, Fallback qua Main Queue)
void privatized_pool_emplace(PrivatizedThreadPool* pool, TaskFunc func, void* arg, TaskPriority prio, CoreType core);

// Hủy Pool
void privatized_pool_destroy(PrivatizedThreadPool* pool);

// ----------------------------------------------------------------------------
// APIS PUBLIC BỔ SUNG (NÂNG CẤP 1, 2, 3, 4)
// ----------------------------------------------------------------------------

// Nâng cấp 3: SIMD Task Batching ($O(1)$ Lock-Free Push nhiều Task)
void privatized_pool_emplace_batch(PrivatizedThreadPool* pool, const TaskItem* items, size_t count);

// Nâng cấp 3: Dequeue theo Batch ra L1 Cache để xử lý Vector SIMD (AVX2 / AVX-512)
size_t privatized_pool_dequeue_batch(Worker* worker, TaskItem* out_batch, size_t max_batch);

// Nâng cấp 4: Submit Async I/O (Read/Write) thẳng xuống Kernel Arch Linux qua io_uring
bool privatized_pool_submit_io_read(Worker* worker, int fd, void* buf, size_t bytes, off_t offset, TaskFunc on_complete, void* user_data);
