#ifndef MATRIX_DIV_H
#define MATRIX_DIV_H

#include "Matrix.h"

// ==========================================================
// 1. PHÉP CHIA MA TRẬN TĨNH AN TOÀN (STACK)
// ==========================================================

/**
 * Chia ma trận 3x3 cho một hằng số (Safe Scalar Division)
 * Nếu hằng số bằng 0, trả về ma trận toàn ô 0.0f để tránh sập hệ thống.
 */
static inline Mat3x3 mat3x3_div_scalar(const Mat3x3* A, float scalar) 
{
    Mat3x3 res;
    if (fabsf(scalar) < 1e-6f) {
        memset(res.m, 0, 9 * sizeof(float));
        return res;
    }
    float inv = 1.0f / scalar;
    for (int i = 0; i < 9; i++) res.m[i] = A->m[i] * inv;
    return res;
}

/**
 * Chia ma trận 2x2 cho một hằng số An toàn
 */
static inline Mat2x2 mat2x2_div_scalar(const Mat2x2* A, float scalar) 
{
    Mat2x2 res;
    if (fabsf(scalar) < 1e-6f) 
    {
        memset(res.m, 0, 4 * sizeof(float));
        return res;
    }

    float inv = 1.0f / scalar;

    for (int i = 0; i < 4; i++) res.m[i] = A->m[i] * inv;
    return res;
}


// ==========================================================
// 2. PHÉP CHIA MẢNG PHẲNG ĐỊA HÌNH AN TOÀN (FloatMatrix View)
// ==========================================================

/**
 * Chia ma trận thành phần: dest = dest / src (Element-wise Division)
 * Bảo vệ tuyệt đối: Nếu ô nào của src bằng 0, ô tương ứng ở dest sẽ gán bằng 0.0f.
 */
static inline void mat_div_safe(FloatMatrix* dest, const FloatMatrix* src) 
{
    int total_cells = dest->width * dest->height;

    for (int i = 0; i < total_cells; i++) {
        float denominator = src->data[i];
        dest->data[i] = (denominator != 0.0f) ? (dest->data[i] / denominator) : 0.0f;
    }
}

/**
 * Chia mảng phẳng cho một hằng số: m = m / scalar
 */
static inline void mat_scale_div(FloatMatrix* m, float scalar) 
{
    if (fabsf(scalar) < 1e-6f) {
        memset(m->data, 0, m->width * m->height * sizeof(float));
        return;
    }

    float inv = 1.0f / scalar;
    int total_cells = m->width * m->height;

    for (int i = 0; i < total_cells; i++) 
    {
        m->data[i] *= inv;
    }
}

/**
 * Chuẩn hóa trường vận tốc (Normalize Vector Field) từ 2 mảng chứa vận tốc U (X) và V (Y):
 * Hỗ trợ thuật toán Advection (Dịch chuyển chất lỏng) của Navier-Stokes.
 * Công thức: U = U / sqrt(U^2 + V^2), V = V / sqrt(U^2 + V^2)
 */
static inline void mat_normalize_field(FloatMatrix* U, FloatMatrix* V) 
{
    int total_cells = U->width * U->height;

    for (int i = 0; i < total_cells; i++) 
    {
        float u_val = U->data[i];
        float v_val = V->data[i];
        float length = sqrtf(u_val * u_val + v_val * v_val);
        
        if (length > 1e-6f) {
            float inv_len = 1.0f / length;
            U->data[i] *= inv_len;
            V->data[i] *= inv_len;
        } else {
            U->data[i] = 0.0f;
            V->data[i] = 0.0f;
        }
    }
}

#endif // MATRIX_DIV_H