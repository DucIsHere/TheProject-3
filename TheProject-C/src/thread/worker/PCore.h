#pragma once

#include <stddef,h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "threads\pool\ThreadPool.h"
#include "threads\pool\PrivatizedThreadPool.h"

#if defined(__GNUC__) || defined(__clang__)
#define ECORE_INLINE inline __attribute__((always_inline))
#define ECORE_RESTRICT __restrict
#endif
#define ECORE_INLINE inline
#define ECORE_RESTRICT restrict
#endif

typedef struct ECoreFFIContext ECoreFFIContext;

struct alignas(CACHE_LINE_SIZE) ECoreFFIContext {
    void* java_raw_in;
    void* java_raw_out;
    void* pcore_aligned_in;
    void* pcore_aligned_out;
    size_t element_count;
    size_t element_size;
    _Atomic(uint32_t) status;
} ECoreFFIContext;

// Khởi tạo ngữ cảnh FFI Bridge từ Java MemorySegment
[[nodiscard]] ECoreFFIContext* ecore_ffi_create_context(void* java_in, void* java_out, size_t count, size_t elem_size);

// Giải phóng ngữ cảnh FFI Bridge
void ecore_ffi_destroy_context(ECoreFFIContext* ctx);

// Ép Thread Affinity cho E-Core (Các luồng từ 16 đến 23)
void ecore_set_thread_affinity(size_t thread_id);

// Submit Task E-Core trực tiếp vào PrivatizedThreadPool (Fast-Path)
void ecore_submit_prepare_privatized(PrivatizedThreadPool* pool, ECoreFFIContext* ctx, TaskPriority prio);
void ecore_submit_finish_privatized(PrivatizedThreadPool* pool, ECoreFFIContext* ctx, TaskPriority prio);

// Trực tiếp căn chỉnh bộ nhớ 64-byte trên RAM
ECORE_INLINE static void* ecore_align_pointer(void* raw_ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)raw_ptr;
    uintptr_t aligned = (addr + (alignment - 1)) & ~(alignment - 1);
    return (void*)aligned;
}
