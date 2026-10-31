#ifndef MATRIX_ROTATE_Y_H
#define MATRIX_ROTATE_Y_H

#include <immintrin.h>

#include "Matrix.h"
#include "MatrixFastMath.h"

static inline Mat2x2 mat2x2_1d_rotate_y(float radians)
{
     float c = fm_cos(radians);
     Mat2x2 res;
     res.m[0] = c; res.m[1] = 0.0f;
     res.m[2] = 0.0f; res.m[3] = 1.0f;
     return res;
}

static inline Mat2x2 mat2x2_2d_rotate_y(float radians)
{
     float c = fm_cos(radians);
     float s = fm_sin(radians);
     Mat2x2 res;
     res.m[0] = c; res.m[1] = s;
     res.m[2] = -s; res.m[3] = c;
     return res;
}

static inline Mat2x2 mat2x2_3d_rotate_y(float radians)
{
     float c = fm_cos(radians);
     Mat2x2 res;
     res.m[0] = c;    res.m[1] = 0.0f;
     res.m[2] = 0.0f; res.m[3] = c;
     return res;
}

static inline Mat2x1 mat2x1_1d_rotate_y(const Mat2x1* V, float radians) 
{
    float c = fm_cos(radians);
    Mat2x1 res;
    res.m[0] = V->m[0] * c;
    res.m[1] = V->m[1];
    return res;
}

static inline Mat2x1 mat2x1_2d_rotate_y(const Mat2x1* V, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat2x1 res;
    res.m[0] = V->m[0] * c + V->m[1] * s;
    res.m[1] = -V->m[0] * s + V->m[1] * c;
    return res;
}

static inline Mat2x1 mat2x1_3d_rotate_y(const Mat2x1* V, float radians) 
{
    // SSE Tối ưu luồng tính toán song song cho Vector dẹt 2x1
    __m128 vec = _mm_set_ps(0.0f, 0.0f, V->m[1], V->m[0]);
    float c = fm_cos(radians);
    __m128 scale = _mm_set_ps(1.0f, 1.0f, 1.0f, c);
    __m128 mul = _mm_mul_ps(vec, scale);
    
    Mat2x1 res;
    _mm_store_ss(&res.m[0], mul);
    res.m[1] = V->m[1];
    return res;
}


// ==========================================================
// 3. CẤU TRÚC MA TRẬN 2x3 (Biến đổi hệ tọa độ dẹt)
// ==========================================================

static inline Mat2x3 mat2x3_1d_rotate_y(const Mat2x3* M, float radians) 
{
    float c = fm_cos(radians);
    Mat2x3 res;
    res.m[0] = M->m[0] * c; res.m[1] = M->m[1]; res.m[2] = M->m[2] * c;
    res.m[3] = M->m[3];     res.m[4] = M->m[4]; res.m[5] = M->m[5];
    return res;
}

static inline Mat2x3 mat2x3_2d_rotate_y(const Mat2x3* M, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat2x3 res;
    // Hàng 0 biến đổi xoay theo góc Y phẳng
    res.m[0] = M->m[0] * c - M->m[3] * s;
    res.m[1] = M->m[1] * c - M->m[4] * s;
    res.m[2] = M->m[2] * c - M->m[5] * s;
    // Hàng 1
    res.m[3] = M->m[0] * s + M->m[3] * c;
    res.m[4] = M->m[1] * s + M->m[4] * c;
    res.m[5] = M->m[2] * s + M->m[5] * c;
    return res;
}

static inline Mat2x3 mat2x3_3d_rotate_y(const Mat2x3* M, float radians) 
{
    // Sử dụng AVX2 xử lý hàng loạt phần tử của ma trận 2x3 cùng lúc
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    
    __m256 row0 = _mm256_set_ps(0.0f, 0.0f, M->m[2], M->m[1], M->m[0], 0.0f, 0.0f, 0.0f);
    __m256 row1 = _mm256_set_ps(0.0f, 0.0f, M->m[5], M->m[4], M->m[3], 0.0f, 0.0f, 0.0f);
    
    __m256 cos_v = _mm256_set1_ps(c);
    __m256 sin_v = _mm256_set1_ps(s);
    
    __m256 res_row0 = _mm256_fsub_ps(_mm256_mul_ps(row0, cos_v), _mm256_mul_ps(row1, sin_v));
    
    float out_f[8];
    _mm256_storeu_ps(out_f, res_row0);
    
    Mat2x3 res;
    res.m[0] = out_f[3]; res.m[1] = out_f[4]; res.m[2] = out_f[5];
    res.m[3] = M->m[3];  res.m[4] = M->m[4];  res.m[5] = M->m[5];
    return res;
}


// ==========================================================
// 4. CẤU TRÚC MA TRẬN KHÔNG GIAN KHỦNG 3x3
// ==========================================================

static inline Mat3x3 mat3x3_1d_rotate_y(float radians) 
{
    float c = fm_cos(radians);
    Mat3x3 res = {0};
    res.m[0] = c; res.m[4] = 1.0f; res.m[8] = 1.0f;
    return res;
}

static inline Mat3x3 mat3x3_2d_rotate_y(float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x3 res = {0};
    res.m[0] = c;  res.m[2] = s;
    res.m[4] = 1.0f;
    res.m[6] = -s; res.m[8] = c;
    return res;
}

static inline Mat3x3 mat3x3_3d_rotate_y(float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x3 res;
    res.m[0] = c;    res.m[1] = 0.0f; res.m[2] = s;
    res.m[3] = 0.0f; res.m[4] = 1.0f; res.m[5] = 0.0f;
    res.m[6] = -s;   res.m[7] = 0.0f; res.m[8] = c;
    return res;
}


// ==========================================================
// 5. CẤU TRÚC MA TRẬN 3x1 (Vector không gian 3D)
// ==========================================================

static inline Mat3x1 mat3x1_1d_rotate_y(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians);
    Mat3x1 res;
    res.m[0] = V->m[0] * c; res.m[1] = V->m[1]; res.m[2] = V->m[2];
    return res;
}

static inline Mat3x1 mat3x1_2d_rotate_y(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x1 res;
    res.m[0] = V->m[0] * c + V->m[2] * s;
    res.m[1] = V->m[1];
    res.m[2] = -V->m[0] * s + V->m[2] * c;
    return res;
}

static inline Mat3x1 mat3x1_3d_rotate_y(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x1 res;
    res.m[0] =  c * V->m[0] + s * V->m[2];
    res.m[1] = V->m[1]; // Trục Y giữ nguyên
    res.m[2] = -s * V->m[0] + c * V->m[2];
    return res;
}


// ==========================================================
// 6. CẤU TRÚC MA TRẬN 3x2 (Ma trận chuyển cơ sở hình chiếu)
// ==========================================================

static inline Mat3x2 mat3x2_1d_rotate_y(const Mat3x2* M, float radians) 
{
    float c = fm_cos(radians);
    Mat3x2 res;
    res.m[0] = M->m[0] * c; res.m[1] = M->m[1] * c;
    res.m[2] = M->m[2];     res.m[3] = M->m[3];
    res.m[4] = M->m[4];     res.m[5] = M->m[5];
    return res;
}

static inline Mat3x2 mat3x2_2d_rotate_y(const Mat3x2* M, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x2 res;
    res.m[0] = c * M->m[0] + s * M->m[4];
    res.m[1] = c * M->m[1] + s * M->m[5];
    res.m[2] = M->m[2];
    res.m[3] = M->m[3];
    res.m[4] = -s * M->m[0] + c * M->m[4];
    res.m[5] = -s * M->m[1] + c * M->m[5];
    return res;
}

static inline Mat3x2 mat3x2_3d_rotate_y(const Mat3x2* M, float radians) 
{
    float c = fm_cos(radians);
    float s = fm_sin(radians);
    Mat3x2 res;
    // Đồng bộ hóa cấu trúc 3D đầy đủ trên khung 3x2 tĩnh
    res.m[0] = c * M->m[0] + s * M->m[4];
    res.m[1] = c * M->m[1] + s * M->m[5];
    res.m[2] = M->m[2];
    res.m[3] = M->m[3];
    res.m[4] = -s * M->m[0] + c * M->m[4];
    res.m[5] = -s * M->m[1] + c * M->m[5];
    return res;
}

// ==========================================================
// 7. KÉO HẠ CHIỀU (DOWN-PROJECTION): 3D -> 2D -> 1D + ROTATE Y
// ==========================================================

/**
 * Hạ chiều Ma trận vuông: 3x3 -> 2x2 (Trích xuất mặt phẳng cắt XZ) + Xoay Y
 */
static inline Mat2x2 mat3x3_to_2x2_rotate_y(const Mat3x3* M, float radians) 
{
    Mat2x2 flattened;
    flattened.m[0] = M->m[0]; flattened.m[1] = M->m[2];
    flattened.m[2] = M->m[6]; flattened.m[3] = M->m[8];
    
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat2x2 res;
    res.m[0] = flattened.m[0] * c + flattened.m[1] * s;
    res.m[1] = -flattened.m[0] * s + flattened.m[1] * c;
    res.m[2] = flattened.m[2] * c + flattened.m[3] * s;
    res.m[3] = -flattened.m[2] * s + flattened.m[3] * c;
    return res;
}

/**
 * Hạ chiều Vector: 3x1 -> 2x1 (Bỏ trục Y không gian) + Xoay Y
 */
static inline Mat2x1 mat3x1_to_2x1_rotate_y(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat2x1 res;
    res.m[0] = V->m[0] * c + V->m[2] * s;
    res.m[1] = -V->m[0] * s + V->m[2] * c;
    return res;
}

/**
 * Hạ chiều tuyệt đối về 1D Scalar: 3x1 -> 1D hình chiếu XZ phẳng + Xoay Y
 */
static inline float mat3x1_to_1d_rotate_y(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    return V->m[0] * c + V->m[2] * s;
}

/**
 * Hạ chiều Ma trận chữ nhật: 3x2 -> 2x2 (Trích xuất hệ phẳng) + Xoay Y
 */
static inline Mat2x2 mat3x2_to_2x2_rotate_y(const Mat3x2* M, float radians) 
{
    Mat2x2 flattened;
    flattened.m[0] = M->m[0]; flattened.m[1] = M->m[1];
    flattened.m[2] = M->m[4]; flattened.m[3] = M->m[5];
    
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat2x2 res;
    res.m[0] = flattened.m[0] * c + flattened.m[2] * s;
    res.m[1] = flattened.m[1] * c + flattened.m[3] * s;
    res.m[2] = -flattened.m[0] * s + flattened.m[2] * c;
    res.m[3] = -flattened.m[1] * s + flattened.m[3] * c;
    return res;
}


// ==========================================================
// 8. NÂNG ĐẨY CHIỀU (UP-EMBEDDING): 1D -> 2D -> 3D + ROTATE Y
// ==========================================================

/**
 * Nâng chiều từ 1D Scalar -> 3x1 + Xoay Y
 */
static inline Mat3x1 mat1d_to_3x1_rotate_y(float val_1d, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x1 res;
    res.m[0] = val_1d * c;
    res.m[1] = 0.0f;
    res.m[2] = val_1d * (-s);
    return res;
}

/**
 * Nâng chiều từ Vector phẳng: 2x1 -> 3x1 (Chèn cao độ height_y) + Xoay Y
 */
static inline Mat3x1 mat2x1_to_3x1_rotate_y(const Mat2x1* V, float height_y, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x1 res;
    res.m[0] = V->m[0] * c + V->m[1] * s; 
    res.m[1] = height_y;                  
    res.m[2] = -V->m[0] * s + V->m[1] * c;
    return res;
}

/**
 * Nâng chiều ma trận vuông dẹt: 2x2 -> 3x3 (Khôi phục ma trận không gian) + Xoay Y
 */
static inline Mat3x3 mat2x2_to_3x3_rotate_y(const Mat2x2* M, float radians) 
{
    Mat3x3 embedded = {0};
    embedded.m[0] = M->m[0]; embedded.m[2] = M->m[1];
    embedded.m[4] = 1.0f;    
    embedded.m[6] = M->m[2]; embedded.m[8] = M->m[3];
    
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x3 res;
    res.m[0] = embedded.m[0] * c; res.m[1] = 0.0f; res.m[2] = embedded.m[2] * c;
    res.m[3] = 0.0f;              res.m[4] = 1.0f; res.m[5] = 0.0f;
    res.m[6] = embedded.m[0] * -s;res.m[7] = 0.0f; res.m[8] = embedded.m[2] * -s;
    return res;
}

/**
 * Nâng chiều ma trận hệ số: 2x3 -> 3x3 + Xoay Y
 */
static inline Mat3x3 mat2x3_to_3x3_rotate_y(const Mat2x3* M, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x3 res;
    res.m[0] = M->m[0] * c - M->m[3] * s; res.m[1] = M->m[1] * c - M->m[4] * s; res.m[2] = M->m[2] * c - M->m[5] * s;
    res.m[3] = 0.0f;                      res.m[4] = 1.0f;                      res.m[5] = 0.0f;
    res.m[6] = M->m[0] * s + M->m[3] * c; res.m[7] = M->m[1] * s + M->m[4] * c; res.m[8] = M->m[2] * s + M->m[5] * c;
    return res;
}

#endif
