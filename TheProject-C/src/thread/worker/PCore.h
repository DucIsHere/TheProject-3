#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "ThreadPool.h"

// =============================================================================
// P-CORE RANGE CONFIGURATION (THREAD_0 -> THREAD_15)
// =============================================================================
#define PCORE_RANGE_1_START THREAD_0
#define PCORE_RANGE_1_END   THREAD_15

// Kiểm tra xem 1 Thread ID có thuộc P-Core hay không
static inline bool is_pcore_thread(size_t thread_id) {
    return (thread_id <= (size_t)PCORE_RANGE_1_END) || (thread_id >= 24);
}

// Lấy CoreType từ Thread ID
static inline CoreType pcore_get_type(size_t thread_id) {
    return is_pcore_thread(thread_id) ? CORE_TYPE_PCORE : CORE_TYPE_ECORE;
}
