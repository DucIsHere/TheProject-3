#ifndef MATRIX_ROTATE_Z_H
#define MATRIX_ROTATE_Z_H

#include "Matrix.h"
#include "MatrixFastMath.h"

#include <immintrin.h>

// ==========================================================
// 1. CẤU TRÚC MA TRẬN TĨNH XOAY QUANH Z THEO PHÂN LỚP HỆ CHIỀU
// ==========================================================

static inline Mat2x2 mat2x2_2d_rotate_z(float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat2x2 res;
    res.m[0] = c;    res.m[1] = s;
    res.m[2] = -s;   res.m[3] = c;
    return res;
}

static inline Mat3x3 mat3x3_3d_rotate_z(float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x3 res;
    res.m[0] = c;    res.m[1] = s;    res.m[2] = 0.0f;
    res.m[3] = -s;   res.m[4] = c;    res.m[5] = 0.0f;
    res.m[6] = 0.0f; res.m[7] = 0.0f; res.m[8] = 1.0f;
    return res;
}

static inline Mat3x1 mat3x1_3d_rotate_z(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    // SSE nạp 4 phần tử [0.0, Z, Y, X]
    __m128 v = _mm_set_ps(0.0f, V->m[2], V->m[1], V->m[0]);
    __m128 cos_v = _mm_set1_ps(c);
    __m128 sin_v = _mm_set1_ps(s);
    
    // v_yx = [0.0, 0.0, X, Y] phục vụ xoay XY quanh tâm góc Z
    __m128 v_yx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 0, 1));
    
    __m128 m0 = _mm_mul_ps(v, cos_v);
    __m128 m1 = _mm_mul_ps(v_yx, sin_v);
    
    float out_f[4]; _mm_storeu_ps(out_f, m0);
    float out_f2[4]; _mm_storeu_ps(out_f2, m1);
    
    Mat3x1 res;
    res.m[0] = out_f[0] + out_f2[0]; // c*X + s*Y
    res.m[1] = out_f[1] - out_f2[1]; // c*Y - s*X
    res.m[2] = V->m[2]; // Trục Z giữ nguyên vị trí sâu
    return res;
}


// ==========================================================
// 2. KÉO HẠ CHIỀU (DOWN-PROJECTION): 3D -> 2D -> 1D + ROTATE Z
// ==========================================================

static inline Mat2x2 mat3x3_to_2x2_rotate_z(const Mat3x3* M, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    // Trích xuất phẳng XY bằng lệnh ép kiểu vùng nhớ SSE
    __m128 flat = _mm_set_ps(M->m[4], M->m[3], M->m[1], M->m[0]);
    __m128 flat_shuf = _mm_shuffle_ps(flat, flat, _MM_SHUFFLE(2, 3, 0, 1));
    
    __m128 cos_v = _mm_set1_ps(c);
    __m128 sin_v = _mm_set1_ps(s);
    
    __m128 res_v = _mm_fmsub_ps(_mm_mul_ps(flat, cos_v), _mm_mul_ps(flat_shuf, sin_v), _mm_setzero_ps());
    
    Mat2x2 res;
    float out_f[4];
    _mm_storeu_ps(out_f, res_v);
    res.m[0] = out_f[0]; res.m[1] = out_f[1];
    res.m[2] = out_f[2]; res.m[3] = out_f[3];
    return res;
}

static inline Mat2x1 mat3x1_to_2x1_rotate_z(const Mat3x1* V, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat2x1 res;
    res.m[0] = V->m[0] * c + V->m[1] * s;
    res.m[1] = -V->m[0] * s + V->m[1] * c;
    return res;
}

static inline float mat3x1_to_1d_rotate_z(const Mat3x1* V, float radians) 
{
    return V->m[0] * fm_cos(radians) + V->m[1] * fm_sin(radians);
}


// ==========================================================
// 3. NÂNG ĐẨY CHIỀU (UP-EMBEDDING): 1D -> 2D -> 3D + ROTATE Z
// ==========================================================

static inline Mat3x1 mat2x1_to_3x1_rotate_z(const Mat2x1* V, float fixed_z, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    Mat3x1 res;
    res.m[0] = V->m[0] * c + V->m[1] * s;
    res.m[1] = -V->m[0] * s + V->m[1] * c;
    res.m[2] = fixed_z;
    return res;
}

static inline Mat3x3 mat2x2_to_3x3_rotate_z(const Mat2x2* M, float radians) 
{
    float c = fm_cos(radians); float s = fm_sin(radians);
    // Ép luồng dữ liệu song song đa nhân AVX2
    __m256 row_xy = _mm256_set_ps(0.0f, 0.0f, M->m[3], M->m[2], 0.0f, 0.0f, M->m[1], M->m[0]);
    __m256 cos_v = _mm256_set1_ps(c);
    
    __m256 res_v = _mm256_mul_ps(row_xy, cos_v);
    float out_f[8];
    _mm256_storeu_ps(out_f, res_v);
    
    Mat3x3 res = {0};
    res.m[0] = out_f[0] - (M->m[1] * s); res.m[1] = (M->m[0] * s) + out_f[1];
    res.m[3] = out_f[4] - (M->m[3] * s); res.m[4] = (M->m[2] * s) + out_f[5];
    res.m[8] = 1.0f; // Trục sâu tĩnh bảo toàn
    return res;
}

#endif // MATRIX_ROTATE_Z_H
