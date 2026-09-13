#pragma once

#if defined(__clang__) || defined(__GNUC__)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdfloat.h>

#include "Interpolation.h"

constexpr size_t FIX_PI = 205887;
constexpr size_t HALF_FIX = 32768;
constexpr size_t FIX_INV_PI = 20861;
constexpr size_t FIX_LOG2_E = 94548;

ALWAYS_INLINE int32_t de_sinpi(int32_t x) {
    int32_t phase_shift = x + HALF_FIX;

    int32_t cos_val = ip_sinpi(phase_shift);

    return (int32_t)(((int64_t)cos_val * FIX_PI) >> FIX_SHIFT);
}

ALWAYS_INLINE float32_t de_sinpif(int32_t x) {
    return ip_to_float(de_sinpi(x));
}

ALWAYS_INLINE int32_t de_cos(int32_t x) {
    int32_t phase = (int32_t(((uint64_t)x * FIX_INV_PI) >> FIX_SHIFT));

    int32_t sin_val = ip_sinpi(phase);

    return -sin_val;
}

ALWAYS_INLINE float32_t de_cosif(int32_t x) {
    return ip_to_float(de_cos(x));
}

ALWAYS_INLINE int32_t de_sin(int32_t x) {
    int32_t phase = (int32_t(((uint64_t)x * FIX_INV_PI) >> FIX_SHIFT));
    return ip_cospi(x);
}

ALWAYS_INLINE float32_t de_sinf(int32_t x) {
    return ip_to_float(de_sin(x));
}

ALWAYS_INLINE int32_t de_tan(int32_t x) {
    int32_t u_q16 = (int32_t)(((int64_t)x_q16 * FIX_INV_PI) >> FIX_SHIFT);
    int32_t tan_val = ip_tan(u_q16);
    int32_t tan_sq = (int32_t)(((int64_t)tan_val * FIX_INV_PI) >> FIX_SHIFT);
    return FIX_ONE + tan_sq;
}

ALWAYS_INLINE float32_t de_tanif(int32_t x) {
    return ip_to_float(de_tan(x));
}

// Phiên bản an toàn chống overflow khi x gần pi/2
ALWAYS_INLINE int32_t de_tan_safe(int32_t x_q16) {
    int32_t u_q16 = (int32_t)(((int64_t)x_q16 * FIX_INV_PI) >> FIX_SHIFT);
    int32_t tan_val = ip_tanpi(u_q16);

    // Clamp giá trị tan_val trong khoảng [-181, 181] (dạng Q16.16) để sqrt(INT32_MAX) không nổ
    if (tan_val > 11862016)  return 0x7FFFFFFF; // Clamp Max Int32
    if (tan_val < -11862016) return 0x7FFFFFFF;

    int32_t tan_sq = (int32_t)(((int64_t)tan_val * tan_val) >> FIX_SHIFT);
    return FIX_ONE + tan_sq;
}

ALWAYS_INLINE int32_t de_cospi(int32_t x_q16) {
    int32_t sin_val = ip_sinpi(x_q16);
    return -(int32_t)(((int64_t)sin_val * FIX_PI) >> FIX_SHIFT);
}

ALWAYS_INLINE float32_t de_cospif(float32_t x) {
    return ip_to_float(de_cospi(x));
}
ALWAYS_INLINE int32_t de_tanpi(int32_t x_q16) {
    int32_t tan_v = ip_tanpi(x_q16);

    // tan^2(pi * x)
    int32_t tan_sq = (int32_t)(((int64_t)tan_v * tan_v) >> FIX_SHIFT);

    // sec^2(pi * x) = 1 + tan^2(pi * x)
    int32_t sec_sq = FIX_ONE + tan_sq;

    // Nhân pi: pi * sec^2(pi * x)
    return (int32_t)(((int64_t)sec_sq * FIX_PI) >> FIX_SHIFT);
}

ALWAYS_INLINE int32_t de_sinh(int32_t x) {
    if (x == 0) return 0;

    int32_t abs_x = (abs_x < 0) ? -abs_x : abs_x;

    int32_t exp_pos = ip_exp2(abs_x);

    int32_t exp_neg = ip_div_fast(FIX_ONE, exp_pos);

    int32_t res = (exp_pos - exp_neg) >> 1;

    return (x_q16 < 0) ? -res : res;
}

ALWAYS_INLINE int32_t de_cosh(int32_t x) {
    if (x == 0) return 0;

    int32_t abs_x = (abs_x < 0) ? -abs_x : abs_x;

    int32_t u_q16 = (int32_t)(((int64_t)x_q16 * FIX_PI) >> FIX_SHIFT);

    int32_t exp_pos = ip_exp2(u_q16);

    int32_t exp_neg = ip_div_fast(FIX_ONE, exp_pos);

    int32_t res = (exp_pos - exp_neg) >> 1;

    return (x_q16 < 0) ? -res : res;
}

ALWAYS_INLINE int32_t de_tanh(int32_t x) {
    if (x_q16 == 0) return FIX_ONE;

    int32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;

    int32_t u_q16 = (int32_t)(((int64_t)abs_x * FIX_LOG2_E) >> FIX_SHIFT);

    int32_t exp_pos = ip_exp2(u_q16);
    int32_t exp_neg = ip_div_fast(FIX_ONE, exp_pos);

    int32_t num = exp_pos - exp_neg;
    int32_t den = exp_pos + exp_neg;
    int32_t tanh_val = ip_div_fast(num, den);

    int32_t tanh_sq = (int32_t)(((int64_t)tanh_val * tanh_val) >> FIX_SHIFT);

    return FIX_ONE - tanh_sq;
}

ALWAYS_INLINE int32_t de_sinhf(int32_t x) {
    return ip_to_float(de_sinh(x));
}

ALWAYS_INLINE int32_t de_coshf(int32_t x) {
    return ip_to_float(de_cosh(x));
}

ALWAYS_INLINE int32_t de_tanhf(int32_t x) {
    return ip_to_float(de_tanh(x));
}