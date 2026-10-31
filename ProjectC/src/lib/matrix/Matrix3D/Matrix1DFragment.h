#ifndef MATRIX_1D_FRAGMENT_H
#define MATRIX_1D_FRAGMENT_H

#include <stdint.h>
#include <immintrin.h>

#include "Matrix.h"

#if defined(_STDC_VERSION_) && _STDC_VERSION_ >= 201112L
    #include <stdalign.h>
    #define ALIGN32 alignas(32)
#else defined(_MSC_VER)
    #define ALIGN32 __declspec(align(32))
#else
    #define ALIGN32 __attribute__((aligned(32)))
#endif

typedef struct
{
     __m128 vec;
     uint8_t y, z;
     uint8_t bitmasks;
} FragX3;

typedef struct
{
     __m128 vec;
     uint8_t x, z;
     uint8_t bitmasks;
} FragX2;

static inline uint32_t frag_calc_bitmask(uint8_t y, uint8_t z, uint8_t max_y) 
{
    uint8_t fiber_index = (z * max_y) + y;
    return (1U << fiber_index);
}

static inline FragX3 mat3x3x3_get_frag_x(const Mat3x3x3* m, uint8_t y, uint8_t z)
{
     FragX3 frag;
     frag.y = y;
     frag.z = z;
     frag.bitmasks = frag_calc_bitmask(y, z, 3);
     uint8_t idx = mat3x3x3_index(0, y, z);
     frag.vec = _mm_loadu_ps(&m->data[idx]);
     return frag;
}

static inline FragX3 mat3x2x3_get_frag_x(const Mat3x2x3* m, uint8_t y, uint8_t z)
{
     FragX3 frag;
     frag.y = y;
     frag.z = z;
     frag.bitmasks = frag_calc_bitmask(y, z, 2);
     uint8_t idx = mat3x2x3_index(0, y, z);
     frag.vec = _mm_loadu_ps(&m->data[idx]);
     return frag;
}

static inline FragX2 mat2x2x2_get_frag_z(const Mat2x2x2* m, uint8_t y, uint8_t z)
{
     FragX2 frag;
     frag.z = z;
     frag.y = y;
     frag.bitmasks = frag_calc_bitmask(y, z, 2);
     uint8_t idx = mat2x2x2_index(0, y, z);
     frag.vec = _mm_setzero_ps();
     frag.vec = _mm_loadl_pi(frag.vec, (const __m64*)&m->data[idx]);
     return frag;
}

static inline FragX2 mat2x3x2_get_frag_x(const Mat2x3x2* m, uint8_t y, uint8_t z)
{
     FragX2 frag;
     frag.y = y;
     frag.z = z;
     frag.bitmasks = frag_calc_bitmask(y, z, 3);
     uint8_t idx = mat2x3x2_index(0, y, z);
     frag.vec = _mm_setzero_ps();
     frag.vec = _mm_loadl_pi(frag.vec, (const __m64*)&m->data[idx]);
     return frag;
}

static inline void mat3x3x3_set_frag_x(Mat3x3x3* m, const FragX3* frag)
{
     uint8_t idx = mat3x3x3_index(0, frag->y, frag->z);

     _mm_storeu_si32(&m->data[idx], _mm_castps_si128(frag->vec));
     _mm_storeu_si32(&m->data[idx + 1], _mm_castps_si128(_mm_shuffle_ps(frag->vec, frag->vec, _MM_SHUFFLE(1, 1, 1, 1))));
     _mm_storeu_si32(&m->data[idx + 2], _mm_castps_si128(_mm_shuffle_ps(frag->vec, frag->vec, _MM_SHUFFLE(2, 2, 2, 2,))));
}

typedef void (*FragX3SetFunc)(FragX3* frag, void* user_data);

static inline void mat3x3x3_process_all_x_simd(Mat3x3x3* m, FragX3_SIMD_Callback callback, void* user_data, uint32_t* out_mask) 
{
    uint32_t mask = 0;
    
    // Tự động quét toàn bộ không gian YZ
    for (uint8_t z = 0; z < 3; z++) {
        for (uint8_t y = 0; y < 3; y++) 
        {
            
            // 1. Trích xuất mảnh X3 thẳng vào thanh ghi SIMD
            FragX3_SIMD frag = mat3x3x3_get_frag_x_simd(m, y, z);
            
            // 2. Thực thi hàm tính toán SIMD của bác
            if (callback) 
            {
                callback(&frag, user_data);
            }
            
            // 3. Ghi kết quả từ thanh ghi SIMD ngược lại RAM
            mat3x3x3_set_frag_x_simd(m, &frag);
            
            // 4. Tích lũy Bitmask
            mask |= frag.bit_mask;
        }
    }
    
    if (out_mask) *out_mask = mask; // Trả về 0x1FF (Đủ 9 sợi 1D)
}

#endif
