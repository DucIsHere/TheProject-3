#ifndef MATRIX_PLUS_H
#define MATRIX_PLUS_H

#include "Matrix.h"

// ==========================================================
// 1. PHÉP TOÁN CHUYỂN VỊ NÂNG CAO (TRANSPOSE)
// ==========================================================

/**
 * Chuyển vị ma trận 2x2: (2x2)^T -> 2x2
 */
static inline Mat2x2 mat2x2_transpose(const Mat2x2* A) {
    Mat2x2 res;
    res.m[0] = A->m[0]; res.m[1] = A->m[2];
    res.m[2] = A->m[1]; res.m[3] = A->m[3];
    return res;
}

/**
 * Chuyển vị ma trận 3x3: (3x3)^T -> 3x3
 */
static inline Mat3x3 mat3x3_transpose(const Mat3x3* A) {
    Mat3x3 res;
    res.m[0] = A->m[0]; res.m[1] = A->m[3]; res.m[2] = A->m[6];
    res.m[3] = A->m[1]; res.m[4] = A->m[4]; res.m[5] = A->m[7];
    res.m[6] = A->m[2]; res.m[7] = A->m[5]; res.m[8] = A->m[8];
    return res;
}

/**
 * Chuyển vị ma trận 2x3 thành 3x2: (2x3)^T -> 3x2
 */
static inline Mat3x2 mat2x3_transpose(const Mat2x3* A) {
    Mat3x2 res;
    res.m[0] = A->m[0]; res.m[1] = A->m[3];
    res.m[2] = A->m[1]; res.m[3] = A->m[4];
    res.m[4] = A->m[2]; res.m[5] = A->m[5];
    return res;
}

/**
 * Chuyển vị ma trận 3x2 thành 2x3: (3x2)^T -> 2x3
 */
static inline Mat2x3 mat3x2_transpose(const Mat3x2* A) {
    Mat2x3 res;
    res.m[0] = A->m[0]; res.m[1] = A->m[2]; res.m[2] = A->m[4];
    res.m[3] = A->m[1]; res.m[4] = A->m[3]; res.m[5] = A->m[5];
    return res;
}


// ==========================================================
// 2. PHÉP TOÁN ĐẠI SỐ TUYẾN TÍNH TRÊN STACK (SCALAR & LERP)
// ==========================================================

/**
 * Cộng một hằng số vào ma trận 3x3
 */
static inline Mat3x3 mat3x3_add_scalar(const Mat3x3* A, float scalar) {
    Mat3x3 res;
    for (int i = 0; i < 9; i++) res.m[i] = A->m[i] + scalar;
    return res;
}

/**
 * Nhân ma trận 3x3 với một hằng số
 */
static inline Mat3x3 mat3x3_scale_scalar(const Mat3x3* A, float scalar) {
    Mat3x3 res;
    for (int i = 0; i < 9; i++) res.m[i] = A->m[i] * scalar;
    return res;
}

/**
 * Nội suy tuyến tính (LERP) giữa 2 trạng thái ma trận 3x3: res = A + t * (B - A)
 * Hỗ trợ tạo chuyển động mượt hoặc hòa trộn vùng chuyển tiếp địa hình.
 */
static inline Mat3x3 mat3x3_lerp(const Mat3x3* A, const Mat3x3* B, float t) {
    Mat3x3 res;
    for (int i = 0; i < 9; i++) {
        res.m[i] = A->m[i] + t * (B->m[i] - A->m[i]);
    }
    return res;
}


// ==========================================================
// 3. TIỆN ÍCH CHO MẢNG PHẲNG ĐỊA HÌNH (FloatMatrix SoA View)
// ==========================================================

/**
 * Ghi đè toàn bộ mảng địa hình bằng một giá trị cố định (Reset mảng nhanh)
 */
static inline void mat_fill(FloatMatrix* m, float value) {
    int total_cells = m->width * m->height;
    for (int i = 0; i < total_cells; i++) {
        m->data[i] = value;
    }
}

/**
 * Giới hạn biên độ (Clamp) toàn bộ mảng địa hình trong khoảng [min, max]
 * Rất quan trọng để khống chế độ cao trần-sàn của bản đồ game.
 */
static inline void mat_clamp(FloatMatrix* m, float min_val, float max_val) {
    int total_cells = m->width * m->height;
    for (int i = 0; i < total_cells; i++) {
        if (m->data[i] < min_val) m->data[i] = min_val;
        else if (m->data[i] > max_val) m->data[i] = max_val;
    }
}

/**
 * Phép toán cộng tích lũy Fused Multiply-Add (MAD): dest = dest + (src * scalar)
 * Ứng dụng: Bồi tụ hoặc xói mòn địa hình dựa trên vector vận tốc dòng chảy.
 */
static inline void mat_mad(FloatMatrix* dest, const FloatMatrix* src, float scalar) {
    int total_cells = dest->width * dest->height;
    for (int i = 0; i < total_cells; i++) {
        dest->data[i] += src->data[i] * scalar;
    }
}

/**
 * Trích xuất hoặc sao chép một vùng chữ nhật (Sub-region) giữa các Chunk
 * Cho phép cập nhật từng ô Grid địa hình cục bộ mà không cần nạp lại cả map.
 */
static inline void mat_copy_rect(FloatMatrix* dest, int dest_x, int dest_y,
                                 const FloatMatrix* src, int src_x, int src_y, 
                                 int width, int height) {
    for (int y = 0; y < height; y++) {
        float* d_ptr = &dest->data[(dest_y + y) * dest->width + dest_x];
        const float* s_ptr = &src->data[(src_y + y) * src->width + src_x];
        memcpy(d_ptr, s_ptr, width * sizeof(float));
    }
}

#endif // MATRIX_PLUS_H