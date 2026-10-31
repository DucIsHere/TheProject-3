#ifndef MATRIX_FAST_MATH_H
#define MATRIX_FAST_MATH_H

#include <stdint.h>
#include <immintrin.h>

#define FM_PI          3.14159265358979323846f
#define FM_HALF_PI     1.57079632679489661923f
#define FM_TWO_PI      6.28318530717958647692f
#define FM_LN2         0.69314718055994530942f
#define FM_INV_LN2     1.44269504088896340736f

// ==========================================================
// 1. TẬP LỆNH SIMD AVX2 - XỬ LÝ HÀNG LOẠT 8 SỐ FLOAT CÙNG LÚC
// ==========================================================

/**
 * SIMD AVX2: Hàm Sin và Cos đồng thời bằng đa thức Minimax
 */
static inline void fm_avx2_sincos(__m256 x, __m256* out_sin, __m256* out_cos)
{
     __m256 inv_twopi = __m256_set1_ps(0.15915494309f);
     __m256 twopi = __m256_set1_ps(FM_TWO_PI);
     __m256 rounded = __m256_round_ps(__m256_mul_ps(x, inv_twopi), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
     x = _mm256_fmsub_ps(rounded, twopi, x); // Giới hạn về [-PI, PI]

    __m256 x2 = _mm256_mul_ps(x, x);
    
    // Đa thức Sin bậc 9
    __m256 s = _mm256_set1_ps(2.75573192e-6f);
    s = _mm256_fmsub_ps(s, x2, _mm256_set1_ps(1.98412698e-4f));
    s = _mm256_fmsub_ps(s, x2, _mm256_set1_ps(8.33333333e-3f));
    s = _mm256_fmsub_ps(s, x2, _mm256_set1_ps(1.66666667e-1f));
    *out_sin = _mm256_fma_ps(_mm256_mul_ps(s, x2), x, x);

    // Đa thức Cos bậc 8
    __m256 c = _mm256_set1_ps(2.48015873e-5f);
    c = _mm256_fmsub_ps(c, x2, _mm256_set1_ps(1.38888889e-3f));
    c = _mm256_fmsub_ps(c, x2, _mm256_set1_ps(4.16666667e-2f));
    c = _mm256_fmsub_ps(c, x2, _mm256_set1_ps(5.00000000e-1f));
    *out_cos = _mm256_fma_ps(c, x2, _mm256_set1_ps(1.0f));
}

/**
 * SIMD AVX2: Hàm Mũ nhanh exp(x) dựa trên phân tách phần nguyên/phần phân số
 */
static inline __m256 fm_avx2_exp(__m256 x)
{
     __m256 lonum = _mm256_set1_ps(-88.3762626649662f);
     __m256 hinum = _mm256_set1_ps(88.3762626649662f);
     x = _mm256_max_ps(_mm256_min_ps(x, hinum), lonum);

     __m256 inv_ln2 = _mm256_set1_ps(FM_INV_LN2);
     __m256 fx = _mm256_round_ps(_mm256_mul_ps(x, inv_ln2), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

     x = _mm256_fmsub_ps(fx, _mm256_set1_ps(0.69314575195f), x);
     x = _mm256_fmsub_ps(fx, _mm256_set1_ps(1.4286068203e-6f), x);

     __m256 p = _mm256_set1_ps(1.388894901e-3f);
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(8.333367984e-3f));
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(4.166666849e-2f));
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(1.666666681e-1f));
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(5.000000000e-1f));
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(1.0f));
     p = _mm256_fma_ps(p, x, _mm256_set1_ps(1.0f));

     __m256i imm = _mm256_cvtps_epi32(fx);
     imm = _mm256_add_epi32(imm, _mm256_set1_epi32(127));
     imm = _mm256_slli_epi32(imm, 23);
     __m256 pow2nd = _mm256_castsi256_ps(imm);

     return _mm256_mul_ps(p, pow2nd);
}

// ==========================================================
// 2. TẬP LỆNH SCALAR - TỰ CHẾ CHO TỪNG PHẦN TỬ ĐƠN LẺ
// ==========================================================

/**
 * Trị tuyệt đối nhanh (Triệt tiêu bit dấu bằng Bitwise AND)
 */
static inline float fm_abs(float x) 
{
    uint32_t i = *(uint32_t*)&x;
    i &= 0x7FFFFFFF; // Tắt bit dấu ngoài cùng bên trái
    
    return *(float*)&i;
}

/**
 * Căn bậc hai nghịch đảo siêu tốc (Fast Inverse Square Root) + 1 bước lặp Newton-Raphson
 */
static inline float fm_inv_sqrt(float x) 
{
    float xhalf = 0.5f * x;
    uint32_t i = *(uint32_t*)&x;
    i = 0x5f3759df - (i >> 1); // Phép băm ma thuật Quake
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x); // Sửa sai số

    return x;
}

/**
 * Căn bậc hai nhanh bằng phần cứng Intel/AMD (Triệt tiêu vòng lặp tính toán)
 */
static inline float fm_sqrt(float x) 
{
    __m128 vx = _mm_set_ss(x);

    return _mm_cvtss_f32(_mm_sqrt_ss(vx));
}

/**
 * Hàm làm tròn xuống nhanh (Floor) bằng tập lệnh SSE4.1 tích hợp sẵn
 */
static inline float fm_floor(float x) 
{
    __m128 vx = _mm_set_ss(x);

    return _mm_cvtss_f32(_mm_round_ss(vx, vx, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC));
}

/**
 * Hàm làm tròn lên nhanh (Ceil)
 */
static inline float fm_ceil(float x) 
{
    __m128 vx = _mm_set_ss(x);

    return _mm_cvtss_f32(_mm_round_ss(vx, vx, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC));
}

/**
 * Hàm lượng giác đơn bản Scalar: Sin, Cos, Tan
 */
static inline float fm_sin(float x) 
{
    __m256 vx = _mm256_set1_ps(x);
    __m256 vsin, vcos;
    fm_avx2_sincos(vx, &vsin, &vcos);

    return _mm256_cvtss_f32(vsin);
}

static inline float fm_cos(float x) 
{
    __m256 vx = _mm256_set1_ps(x);
    __m256 vsin, vcos;
    fm_avx2_sincos(vx, &vsin, &vcos);

    return _mm256_cvtss_f32(vcos);
}

static inline float fm_tan(float x) 
{
    __m256 vx = _mm256_set1_ps(x);
    __m256 vsin, vcos;
    fm_avx2_sincos(vx, &vsin, &vcos);
    float s = _mm256_cvtss_f32(vsin);
    float c = _mm256_cvtss_f32(vcos);

    return (c != 0.0f) ? (s / c) : 0.0f;
}

/**
 * Hàm Logarit tự nhiên ln(x) tốc độ cao dựa trên việc bóc tách số mũ bit
 */
static inline float fm_log(float x) 
{
    if (x <= 0.0f) return 0.0f;
    uint32_t val_int = *(uint32_t*)&x;
    int32_t exp = ((val_int >> 23) & 0xFF) - 127;
    
    // Đưa phần trị (mantissa) về khoảng [0.5, 1.0]
    val_int = (val_int & 0x007FFFFF) | 0x3F000000;
    float f = *(float*)&val_int;
    
    // Xấp xỉ đa thức bậc 3 cho ln(f) quanh điểm f = 0.707
    float num = f - 1.0f;
    float denom = f + 1.0f;
    float s = num / denom;
    float s2 = s * s;
    float log_f = s * (2.0f + s2 * (0.666666667f));
    
    return (float)exp * FM_LN2 + log_f;
}

/**
 * Lũy thừa nhanh x^y bằng công thức toán logarit: x^y = exp(y * log(x))
 */
static inline float fm_pow(float x, float y) 
{
    if (x == 0.0f) return 0.0f;
    if (y == 0.0f) return 1.0f;
    
    float lx = fm_log(fm_abs(x));
    __m256 vex = _mm256_set1_ps(y * lx);
    float res = _mm256_cvtss_f32(fm_avx2_exp(vex));
    
    // Xử lý dấu của cơ số âm khi số mũ nguyên lẻ
    if (x < 0.0f && fm_abs(y - fm_floor(y)) < 1e-6f) 
    {
        if (((int32_t)y) & 1) res = -res;
    }
    return res;
}

#endif
