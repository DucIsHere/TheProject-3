#ifndef MATRIX_MULTIPLIER_H
#define MATRIX_MULTIPLIER_H

#include "Matrix.h"

// ==========================================================
// 1. NHÂN HAI MA TRẬN GIỐNG NHAU (SAME TYPE MULTIPLICATION)
// ==========================================================

// Nhân 2x2 với 2x2: dest (2x2) = A (2x2) * B (2x2)
static inline Mat2x2 mat2x2_mul(const Mat2x2* A, const Mat2x2* B) {
    Mat2x2 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[2];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[3];
    res.m[2] = A->m[2]*B->m[0] + A->m[3]*B->m[2];
    res.m[3] = A->m[2]*B->m[1] + A->m[3]*B->m[3];
    return res;
}

// Nhân 3x3 với 3x3: dest (3x3) = A (3x3) * B (3x3)
static inline Mat3x3 mat3x3_mul(const Mat3x3* A, const Mat3x3* B) {
    Mat3x3 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[3] + A->m[2]*B->m[6];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[4] + A->m[2]*B->m[7];
    res.m[2] = A->m[0]*B->m[2] + A->m[1]*B->m[5] + A->m[2]*B->m[8];

    res.m[3] = A->m[3]*B->m[0] + A->m[4]*B->m[3] + A->m[5]*B->m[6];
    res.m[4] = A->m[3]*B->m[1] + A->m[4]*B->m[4] + A->m[5]*B->m[7];
    res.m[5] = A->m[3]*B->m[2] + A->m[4]*B->m[5] + A->m[5]*B->m[8];

    res.m[6] = A->m[6]*B->m[0] + A->m[7]*B->m[3] + A->m[8]*B->m[6];
    res.m[7] = A->m[6]*B->m[1] + A->m[7]*B->m[4] + A->m[8]*B->m[7];
    res.m[8] = A->m[6]*B->m[2] + A->m[7]*B->m[5] + A->m[8]*B->m[8];
    return res;
}


// ==========================================================
// 2. NHÂN HAI MA TRẬN KHÁC NHAU (MIXED TYPE MULTIPLICATION)
// ==========================================================

// [Cặp 1] Nhân 3x2 với 2x3: dest (3x3) = A (3x2) * B (2x3)
static inline Mat3x3 mat3x2_mul_2x3(const Mat3x2* A, const Mat2x3* B) {
    Mat3x3 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[3];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[4];
    res.m[2] = A->m[0]*B->m[2] + A->m[1]*B->m[5];

    res.m[3] = A->m[2]*B->m[0] + A->m[3]*B->m[3];
    res.m[4] = A->m[2]*B->m[1] + A->m[3]*B->m[4];
    res.m[5] = A->m[2]*B->m[2] + A->m[3]*B->m[5];

    res.m[6] = A->m[4]*B->m[0] + A->m[5]*B->m[3];
    res.m[7] = A->m[4]*B->m[1] + A->m[5]*B->m[4];
    res.m[8] = A->m[4]*B->m[2] + A->m[5]*B->m[5];
    return res;
}

// [Cặp 2] Nhân 2x3 với 3x2: dest (2x2) = A (2x3) * B (3x2)
static inline Mat2x2 mat2x3_mul_3x2(const Mat2x3* A, const Mat3x2* B) {
    Mat2x2 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[2] + A->m[2]*B->m[4];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[3] + A->m[2]*B->m[5];
    
    res.m[2] = A->m[3]*B->m[0] + A->m[4]*B->m[2] + A->m[5]*B->m[4];
    res.m[3] = A->m[3]*B->m[1] + A->m[4]*B->m[3] + A->m[5]*B->m[5];
    return res;
}

// [Cặp 3] Nhân 2x3 với 3x3: dest (2x3) = A (2x3) * B (3x3)
static inline Mat2x3 mat2x3_mul_3x3(const Mat2x3* A, const Mat3x3* B) {
    Mat2x3 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[3] + A->m[2]*B->m[6];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[4] + A->m[2]*B->m[7];
    res.m[2] = A->m[0]*B->m[2] + A->m[1]*B->m[5] + A->m[2]*B->m[8];

    res.m[3] = A->m[3]*B->m[0] + A->m[4]*B->m[3] + A->m[5]*B->m[6];
    res.m[4] = A->m[3]*B->m[1] + A->m[4]*B->m[4] + A->m[5]*B->m[7];
    res.m[5] = A->m[3]*B->m[2] + A->m[4]*B->m[5] + A->m[5]*B->m[8];
    return res;
}

// [Cặp 4] Nhân 3x3 với 3x2: dest (3x2) = A (3x3) * B (3x2)
static inline Mat3x2 mat3x3_mul_3x2(const Mat3x3* A, const Mat3x2* B) {
    Mat3x2 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[2] + A->m[2]*B->m[4];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[3] + A->m[2]*B->m[5];

    res.m[2] = A->m[3]*B->m[0] + A->m[4]*B->m[2] + A->m[5]*B->m[4];
    res.m[3] = A->m[3]*B->m[1] + A->m[4]*B->m[3] + A->m[5]*B->m[5];

    res.m[4] = A->m[6]*B->m[0] + A->m[7]*B->m[2] + A->m[8]*B->m[4];
    res.m[5] = A->m[6]*B->m[1] + A->m[7]*B->m[3] + A->m[8]*B->m[5];
    return res;
}

// [Cặp 5] Nhân 2x2 với 2x3: dest (2x3) = A (2x2) * B (2x3)
static inline Mat2x3 mat2x2_mul_2x3(const Mat2x2* A, const Mat2x3* B) {
    Mat2x3 res;
    res.m[0] = A->m[0]*B->m[0] + A->m[1]*B->m[3];
    res.m[1] = A->m[0]*B->m[1] + A->m[1]*B->m[4];
    res.m[2] = A->m[0]*B->m[2] + A->m[1]*B->m[5];

    res.m[3] = A->m[2]*B->m[0] + A->m[3]*B->m[3];
    res.m[4] = A->m[2]*B->m[1] + A->m[3]*B->m[4];
    res.m[5] = A->m[2]*B->m[2] + A->m[3]*B->m[5];
    return res;
}


// ==========================================================
// 3. TOÁN BỔ TRỢ BIẾN ĐỔI HỆ TỌA ĐỘ
// ==========================================================

// Tính định thức (Determinant) của ma trận 2x2
static inline float mat2x2_det(const Mat2x2* A) {
    return A->m[0] * A->m[3] - A->m[1] * A->m[2];
}

// Tính ma trận nghịch đảo 2x2: dest = A^-1. Trả về 0 nếu thất bại (det ~ 0)
static inline int mat2x2_invert(Mat2x2* dest, const Mat2x2* A) {
    float det = mat2x2_det(A);
    if (fabsf(det) < 1e-6f) return 0;
    float inv_det = 1.0f / det;
    
    dest->m[0] =  A->m[3] * inv_det;
    dest->m[1] = -A->m[1] * inv_det;
    dest->m[2] = -A->m[2] * inv_det;
    dest->m[3] =  A->m[0] * inv_det;
    return 1;
}

#endif
