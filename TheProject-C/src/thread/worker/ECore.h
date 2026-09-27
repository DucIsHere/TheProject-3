#pragma once

#include <stddef.h>

#include "threads/pool/ThreadPool.h" // Chứa định nghĩa enum CoreType, TaskPriority và Thread ID

// =============================================================================
// E-CORE RANGE CONFIGURATION (THREAD_16 -> THREAD_23)
// =============================================================================
#define ECORE_START THREAD_16
#define ECORE_END   THREAD_23
#define ECORE_COUNT 8

// Kiểm tra xem 1 Thread ID có thuộc dải E-Core hay không
static inline bool is_ecore_thread(size_t thread_id) {
    return (thread_id >= (size_t)ECORE_START) && (thread_id <= (size_t)ECORE_END);
}

// Chuyển đổi Thread ID thành chỉ số E-Core nội bộ (0 -> 7)
static inline size_t ecore_get_relative_index(size_t thread_id) {
    if (is_ecore_thread(thread_id)) {
        return thread_id - (size_t)ECORE_START;
    }
    return 0; // Fallback
}

// Tự động phân bổ TaskPriority cho E-Core (E-Core thường ưu tiên Task nhẹ/trung bình)
static inline TaskPriority ecore_adjust_priority(TaskPriority priority) {
    if (priority == TASK_PRIO_CRITICAL) {
        return TASK_PRIO_HIGH; // Hạ cấp nhẹ để P-Core xử lý CRITICAL
    }
    return priority;
}