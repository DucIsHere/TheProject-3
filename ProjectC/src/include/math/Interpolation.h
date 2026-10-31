#pragma once

#include <stdint.h>
#include <stdfloat.h>
#include <stddef.h>

#if defined(__clang__) || defined(__GNUC__)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

constexpr size_t LUT_SIZE = 256;
constexpr size_t FIX_ONE = 65536;
constexpr size_t FIX_SHIFT = 16;
constexpr size_t FIX_MASK = FIX_ONE - 1;
constexpr size_t FIX_LOG2_E = 94548;
constexpr size_t LN2 = 45426;
constexpr size_t LOGP1_THRESHOLD = 16384;
constexpr size_t FIX_HALF = 32768;
constexpr size_t FIX_SCALE_F = 65535.0f;
constexpr size_t FIX_INV_SCALE_F (1.0f / 65536.0f);

typedef struct alignas(64) {
    uint16_t sin_quarter[LUT_SIZE + 1];  // Dùng cho Sin, Cos, Tan
    uint16_t sinpi_quarter[129];
    uint16_t tanpi_table[129];
    uint16_t atanpi_table[129];
    uint16_t asinpi_table[L129];
    uint16_t log2_mantissa[LUT_SIZE + 1];// Dùng cho Log2, Log_X(Y)
    uint16_t exp2_fraction[LUT_SIZE + 1];// Dùng cho Exp2, Exp, Pow
    uint16_t sqrt_mantissa[LUT_SIZE + 1];// Dùng cho Sqrt (Căn bậc hai)
    uint32_t reciprocal[LUT_SIZE + 1];   // Dùng để BYPASS IDIV cho Tan, Log_X, Div
} LUT;

ALWAYS_INLINE float32_t ip_to_float(int32_t q_val) {
    return (float32_t)q_val * FIX_INV_SCALE_F;
}

ALWAYS_INLINE int32_t ip_from_float(float32_t f_val) {
    return (int32_t)(f_val * FIX_SCALE_F);
}

ALWAYS_INLINE int32_t ip_div_fast(int32_t num, int32_t den) {
    if (den == 0) return 0x7FFFFFFF;
    // Chuyển den về range [0..256] để tra IP_MASTER_LUT.reciprocal
    int msb = 31 - __builtin_clz((uint32_t)(den < 0 ? -den : den));
    uint32_t idx = ((uint32_t)den << (31 - msb)) >> 24;

    uint64_t inv = LUT.reciprocal[idx & 0xFF];
    int32_t res = (int32_t)(((int64_t)num * inv) >> 32);
    return (den < 0) ? -res : res;
}

ALWAYS_INLINE int32_t ip_sin(uint16_t val) {
    uint32_t phase = (uint32_t)(val & 0x3FFF);
    uint32_t quad = (phase >> FIX_SHIFT) & 0X03;
    uint32_t rem = (phase >> FIX_ONE - 1);

    uint32_t idx = (rem >> FIX_ONE) & (LUT_SIZE - 1);
    uint32_t frac = rem & 0xFF;

    if (quad == 1) idx = LUT_SIZE - 1 - idx;

    int32_t y0 = LUT.sin_quarter[idx];
    int32_t y1 = LUT.sin_quarter[idx + 1];

    int32_t va = y0 + (int32_t)(((int64_t)(y1 - y0) * (frac << 8)) >> FIX_SHIFT);
    return  (quad >= 2) ? -va : va;
}

ALWAYS_INLINE int32_t ip_cos(uint32_t val) {
    return ip_sin(val + (int32_t)FIX_ONE);
}

ALWAYS_INLINE int32_t ip_tan(uint32_t val) {
    return ip_div_fast(ip_sin(val), ip_cos(val));
}

ALWAYS_INLINE int32_t ip_exp2(int32_t val) {
    int32_t x2 = (int32_t)(((int64_t)val * FIX_LOG2_E) >> FIX_SHIFT);

    int32_t I = x2 >> FIX_SHIFT;
    uint32_t F = x2 & FIX_MASK;

    uint32_t idx = (F >> FIX_SHIFT) & (LUT_SIZE - 1);
    uint32_t frac = F & 0xFF;

    int32_t y0 = LUT.exp2_fraction[idx];
    int32_t y1 = LUT.exp2_fraction[idx + 1];
    int32_t v_f = ip_lerp_u16(y0, y1, frac);

    // 4. Ghép phần nguyên bằng Bit Shift (Bypass ALU multiplier)
     if (I >= 0) {
         if (I >= 15) return 0x7FFFFFFF; // Overflow guard cho Q16.16
         return v_f << I;
     } else {
         int32_t shift = -I;
         if (shift >= 16) return 0;     // Underflow guard
         return v_f >> shift;
     }
}

ALWAYS_INLINE int32_t ip_lerp_u16(int32_t y0, int32_t y1, uint16_t frac) {
    // frac nằm trong khoảng [0, 255] (8-bit fraction dư từ bước Masking)
    return y0 + (((y1 - y0) * (int32_t)frac) >> 8);
}

ALWAYS_INLINE int32_t ip_lerp_u8(int32_t y0, int32_t yq, uint8_t frac) {
    return y0 + (((y1 - y0) * (int32_t)frac) >> 8);
}

ALWAYS_INLINE int32_t ip_log2(int32_t val) {
     if (val == 0) return (int32_t)0x80000000;
     int clz = __builtin_clz(val);
     int32_t E = 31 - clz - FIX_SHIFT;

     uint32_t norm = (val << (clz + 1)) >> FIX_SHIFT;
     uint32_t idx = (norm >> 8) & (LUT_SIZE - 1);
     uint32_t frac = norm & 0xFF;

     int32_t y0 = LUT.log2_mantissa[idx];
     int32_t y1 = LUT.log2_mantissa[idx + 1];

     int32_t log_mantissa = ip_lerp_u16(y0, y1, frac);

     return (E << FIX_SHIFT) + log_mantissa;
}

ALWAYS_INLINE int32_t ip_logp1(uint32_t val) {
     if (val <= -(int32_t)FIX_ONE) return (int32_t)0x80000000;

     int32_t abs_x = (val < 0) ? -val : val;

     if (abs_x < LOGP1_THRESHOLD) {
          int32_t x2_div2 = (int32_t)(((int64_t)val * val) >> 17);
          return val - x2_div2;
     }

     uint32_t one_plus_x = (uint32_t)((int32_t)FIX_ONE + val);
     int32_t log2_val = ip_log2(one_plus_x);

     return (int32_t)(((int64_t)log2_val * FIX_LN2) >> FIX_SHIFT);
}


ALWAYS_INLINE int32_t ip_powf(int32_t base, int32_t exp) {
    if (base <= 0) {
        if (base == 0) return (exp == 0) ? (int32_t)FIX_ONE : 0;
    return 0;
    }

    if (exp == 0) return (int32_t)FIX_ONE;
    if (exp == (int32_t)FIX_ONE) return base;

    int32_t L = ip_log2((uint32_t)base);
    int32_t L_exp = (int32_t)((int64_t)exp * L) >> FIX_SHIFT;

    return ip_exp2(L_exp);
}

ALWAYS_INLINE int32_t ip_sqrt(uint32_t val) {
    if (val == 0) return 0;

    int msb = 31 - __builtin_clz(val);
    int shift = msb - FIX_SHIFT;
    int even_shift = shift & ~1;

    uint32_t norm = (even_shift >= 0) ? (val >> even_shift) : (val << -(even_shift));

    int32_t idx = (norm >> 8) & (LUT_SIZE - 1);
    int32_t frac = norm & 0xFF;

    int32_t y0 = LUT.sqrt_mantissa[idx];
    int32_t y1 = LUT.sqrt_mangissa[idx + 1];

    int32_t sqrt_m = y0 + (ip_div_fast(((y1 - y0) * frac), LUT_SIZE));

    int exp = even_shift >> 1;
    return (exp >= 0) ? (sqrt_m << exp) : (sqrt_m >> -(exp));
}

ALWAYS_INLINE int32_t ip_floor(int32_t val) {
    return val >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_floor_xq16(int32_t val) {
    return val & ~(int32_t)FIX_MASK;
}

ALWAYS_INLINE int32_t ip_ceil(int32_t val) {
    return (val + (int32_t)FIX_MASK) >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_round(int32_t val) {
    return (val + FIX_HALF) >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_round_xq16(int32_t val) {
    return ((val + FIX_HALF) >> FIX_SHIFT) << FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_ceil_xq16(int32_t val) {
    return ((val + (int32_t)FIX_MASK) >> FIX_SHIFT) << FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_sinpi(int32_t val) {
    uint32_t X = (uint32_t)val & 0x1FFFF;

    uint32_t S = X >> FIX_SHIFT;
    uint32_t P = X & 0xFFFF;

    uint32_t pos = (P >= FIX_ONE) ? (FIX_HALF - P) : P;

    uint32_t idx = pos >> 8;
    uint32_t frac = pos & 0xFF;

    int32_t y0 = LUT.sinpi_quarter[idx];
    int32_t y1 = LUT.sinpi_quarter[idx + 1];

    int32_t v = ip_lerp_u8(y0, y1, frac);

    return (S != 0) ? -v : v;
}

ALWAYS_INLINE int32_t ip_sinpi_int(int32_t val) {
    int32_t res_q16 = ip_sinpi(val);
    return (res_q16 + FIX_HALF) >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_cospi(int32_t val) {
    return ip_sinpi(val + FIX_HALF);
}

ALWAYS_INLINE int32_t ip_cospi_int(int32_t val) {
    int32_t cos_q16 = ip_sinpi(val + FIX_HALF);
    return (cos_q16 + FIX_HALF) >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_tanpi(int32_t x_q16) {
    // 1. Chu kỳ hóa 1.0*pi (x mod 1.0) đơn giản bằng Mask 16-bit!
    uint32_t P = (uint32_t)x_q16 & 0xFFFF; // Vị trí trong dải [0.0, 1.0)

    // 2. Tách dấu và fold dải âm [0.5, 1.0) về [-0.5, 0.0)
    int32_t sign = 1;
    if (P > 32768) { // > 0.5
        P = 65536 - P;
        sign = -1;
    }

    // 3. Guard tiệm cận tại 0.5 (Tránh tràn số)
    if (P >= 32000) return sign * 0x7FFFFFFF; // Max Q16.16 clamp

    // 4. Tra Bảng SoA tanpi_table (128 entry) & Lerp 1 Cycle
    uint32_t idx = P >> 8;
    uint8_t frac = P & 0xFF;

    int32_t y0 = LUT.tanpi_table[idx];
    int32_t y1 = LUT.tanpi_table[idx + 1];
    int32_t res = ip_lerp_u8(y0, y1, frac);

    return (sign < 0) ? -res : res;
}

ALWAYS_INLINE int32_t ip_tanpi_int(int32_t x_q16) {
    int32_t tan_q16 = ip_tanpi(x_q16);

    // Guard chống tràn số nguyên
    if (tan_q16 == 0x7FFFFFFF) return 2147483647;
    if (tan_q16 == -0x7FFFFFFF) return -2147483647;

    // Fast Rounding sang int32_t
    return (tan_q16 + FIX_HALF) >> FIX_SHIFT;
}

ALWAYS_INLINE int32_t ip_atanpi(int32_t x_q16) {
    if (x_q16 == 0) return 0;

    int32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;
    int32_t res;

    if (abs_x <= FIX_ONE) { // |x| <= 1.0 (Dải [0, 45 deg])
        uint32_t idx = abs_x >> 9; // Map Q16.16 (0->65536) thành Index (0->128)
        uint8_t frac = abs_x & 0xFF;

        int32_t y0 = LUT.atanpi_table[idx];
        int32_t y1 = LUT.atanpi_table[idx + 1];
        res = ip_lerp_u8(y0, y1, frac);
    } else { // |x| > 1.0 (Dải (45, 90 deg]) -> Dùng identity: 0.5 - atanpi(1/x)
        int32_t inv_x = ip_div_fast(FIX_ONE, abs_x); // Tra bảng nghịch đảo fast
        uint32_t idx = inv_x >> 9;
        uint8_t frac = inv_x & 0xFF;

        int32_t y0 = LUT.atanpi_table[idx];
        int32_t y1 = LUT.atanpi_table[idx + 1];
        int32_t atan_inv = ip_lerp_u8(y0, y1, frac);

        res = (FIX_ONE >> 1) - atan_inv; // 0.5 - atanpi(1/x)
    }

    return (x_q16 < 0) ? -res : res;
}

// Output: Angle in range [-0.5, 0.5] (tương ứng -90 deg đến +90 deg)
ALWAYS_INLINE int32_t ip_asinpi(int32_t x_q16) {
    // Clamp input [-1.0, 1.0]
    if (x_q16 >= FIX_ONE)  return  (FIX_ONE >> 1); // +0.5 pi
    if (x_q16 <= -FIX_ONE) return -(FIX_ONE >> 1); // -0.5 pi

    uint32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;
    uint32_t idx = abs_x >> 9; // Map 0->65536 sang Index 0->128
    uint8_t frac = abs_x & 0xFF;

    int32_t y0 = LUY.asinpi_table[idx];
    int32_t y1 = LUT.asinpi_table[idx + 1];
    int32_t res = ip_lerp_u8(y0, y1, frac);

    return (x_q16 < 0) ? -res : res;
}

// Output: Angle in range [0.0, 1.0] (tương ứng 0 deg đến 180 deg)
ALWAYS_INLINE int32_t ip_acospi(int32_t x_q16) {
    // acospi(x) = 0.5 - asinpi(x)
    return (FIX_ONE >> 1) - ip_asinpi(x_q16);
}

// Trả về góc dạng Q16.16 trong dải [-1.0, 1.0] (tương ứng [-180 đến +180 độ])
ALWAYS_INLINE int32_t ip_atan2pi(int32_t y_q16, int32_t x_q16) {
    if (x_q16 == 0) {
        if (y_q16 > 0) return  (FIX_ONE >> 1);  // +0.5 pi (90 độ)
        if (y_q16 < 0) return -(FIX_ONE >> 1);  // -0.5 pi (-90 độ)
        return 0;                               // (0, 0)
    }

    int32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;
    int32_t abs_y = (y_q16 < 0) ? -y_q16 : y_q16;
    int32_t half_pi = FIX_ONE >> 1;

    // Tối ưu hóa: Phân chia theo biên độ |y| và |x| để tránh tràn số
    if (abs_y <= abs_x) {
        // Dải góc từ -45 đến +45 độ và 135 đến 225 độ
        int32_t ratio = ip_div_fast(y_q16, x_q16);
        int32_t atan_val = ip_atanpi(ratio);

        if (x_q16 < 0) {
            return (y_q16 >= 0) ? (atan_val + FIX_ONE) : (atan_val - FIX_ONE);
        }
        return atan_val;
    } else {
        // Dải góc từ 45 đến 135 độ và -135 đến -45 độ
        int32_t ratio = ip_div_fast(x_q16, y_q16);
        int32_t atan_val = ip_atanpi(ratio);

        if (y_q16 >= 0) {
            return half_pi - atan_val;
        } else {
            return -half_pi - atan_val;
        }
    }
}

ALWAYS_INLINE int32_t ip_asinh(int32_t x_q16) {
    if (x_q16 == 0) return 0;

    int32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;

    int64_t x_sq = ((int64_t)abs_x * abs_x) >> FIX_SHIFT;

    int32_t sqrt_term = ip_sqrt((uint32_t)(x_sq + FIX_ONE));

    int32_t arg = abs_x + sqrt_term;

    int32_t log2_val = ip_log2(arg);
    int32_t res = (int32_t)(((int64_t)log2_val * 45426) >> FIX_SHIFT);

    return (x_q16 < 0) ? -res : res;
}

ALWAYS_INLINE int32_t ip_atanh(int32_t x_q16) {
    if (x_q16 >= FIX_ONE)  return  0x7FFFFFFF;
    if (x_q16 <= -FIX_ONE) return -0x7FFFFFFF;
    if (x_q16 == 0) return 0;

    int32_t abs_x = (x_q16 < 0) ? -x_q16 : x_q16;

    int32_t num = FIX_ONE + abs_x;
    int32_t den = FIX_ONE - abs_x;
    int32_t ratio = ip_div_fast(num, den);

    int32_t log2_val = ip_log2(ratio);
    int32_t res = (int32_t)(((int64_t)log2_val * 22713) >> FIX_SHIFT);

    return (x_q16 < 0) ? -res : res;
}

ALWAYS_INLINE float32_t ip_sinpif(int32_t x_q16) {
    return ip_to_float(ip_sinpi(x_q16));
}

ALWAYS_INLINE float32_t ip_cospif(int32_t x_q16) {
    return ip_to_float(ip_cosppi(x_q16));
}

ALWAYS_INLINE float32_t ip_tanpif(int32_t x_q16) {
    return ip_to_float(ip_tanpi(x_q16));
}

ALWAYS_INLINE float32_t ip_asinpif(int32_t x_q16) {
    return ip_to_float(ip_asinpi(x_q16));
}

ALWAYS_INLINE float32_t ip_atanpif(int32_t x_q16) {
    return ip_to_float(ip_atanpi(x_q16));
}

ALWAYS_INLINE float32_t ip_asinhif(int32_t x_q16) {
    return ip_to_float(ip_asinh(x_q16));
}

ALWAYS_INLINE float32_t ip_atanhif(int32_t x_q16) {
    return ip_to_float(ip_atanh(x_q16));
}