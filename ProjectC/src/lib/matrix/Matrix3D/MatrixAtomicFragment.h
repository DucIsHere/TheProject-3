#ifndef MATRIX_ATOMIC_FRAGMENT_H
#define MATRIX_ATOMIC_FRAGMENT_H

#include <stdint.h>
#include <immintrin.h>
#include "Matrix.h"

#if defined(_STDC_VERSION__) && _STDC_VERSION__ >= 201112L
     #include <stdalign.h>
     #define ALIGN32 alignas(32)
#elif defined(_MSC_VER)
     #define ALIGN32 __declspec(align(32))
#else 
     #define ALIGN32 __attribute__((aligned(32)))
#endif

typedef struct 
{
     float val;
     uint8_t x, y, z;
     uint8_t bit_index;
     uint32_t bitmask;
} FragX1;

typedef struct ALIGN32
{
     __m256 vec;
     uint32_t indices[8];
     uint32_t batch_mask;
} FragAtomic;

static inline uint8_t atomic_calc_bit_index(uint8_t x, uint8_t y, uint8_t z)
{
     return x + (y * 3) + (z * 9);
}

static inline uint8_t atomic_calc_bitmask(uint8_t x, uint8_t y, uint8_t z)
{
     return (1U << atomic_calc_bit_index(x, y, z));
}

static inline FragX1 mat3x3x3_get_frag_atomic(Mat3x3x3* m, uint8_t x, uint8_t y, uint8_t z)
{
     FragX1 frag;
     frag.x = x;
     frag.y = y;
     frag.z = z;
     frag.bit_index = atomic_calc_bit_index(x, y, z);
     frag.bitmask = (1U << frag.bit_index);

     uint8_t idx = mat3x3x3_index(x, y, z);
     frag.val = m->data[idx];
     return frag;
}

static inline void mat3x3x3_set_frag_atomic(Mat3x3x3* m, FragX1 frag)
{
     uint8_t idx = mat3x3x3_index(frag.x, frag.y, frag.z);
     m->data[idx] = frag.val;
}

static inline FragAtomic mat3x3x3_get_frag_atomic_vec(const Mat3x3x3* m, const uint8_t coord_xyz[8][3])
{
     FragAtomic ato;
     ato.batch_mask = 0;

     ALIGN32 int32_t byte_offset[8];

     for (int i = 0; i < 8; i++)
     {
          uint8_t x = coord_xyz[i][0];
          uint8_t y = coord_xyz[i][1];
          uint8_t z = coord_xyz[i][2];

          uint8_t bit_idx = atomic_calc_bit_index(x, y, z);
          ato.batch_mask |= (1U << bit_idx);

          uint8_t idx = mat3x3x3_index(x, y, z);
          ato.indices[i] = arrays_idx;
          byte_offset[i] = arrays_idx * sizeof(float);
     }

     __m256i v_offsets = _mm256_load_si256((const __m256i*)byte_offset);
     ato.vec = _mm256_i32gather_ps(m->data, v_offsets, 1);

     return ato;
}

static inline void mat3x3x3_set_atomic_avx8(Mat3x3x3* m, const FragAtomic* batch) 
{
    ALIGN32 float res[8];
    _mm256_store_ps(res, batch->vec);

    // Scatter kết quả đã tính bằng AVX2 về lại từng ô nguyên tử 3D
    for (int i = 0; i < 8; i++) 
    {
        m->data[batch->indices[i]] = res[i];
    }
}


// ==========================================================
// 5. ATOMIC BITWISE AUTO-ITERATOR: VẮT SẠCH 27 HẠT NGUYÊN TỬ
// ==========================================================

typedef void (*Atomic_AVX8_Callback)(FragAtomic* batch, void* user_data);

static inline void mat3x3x3_process_all_atomic_avx2(Mat3x3x3* m, Atomic_AVX8_Callback callback, void* user_data, uint32_t* out_mask) 
{
    uint32_t total_mask = 0;

    // Lưới 27 ô -> Chia làm 3 đợt AVX2 (8 + 8 + 8) và 1 đợt lẻ (3 ô)
    // Hoặc quét theo danh sách tọa độ được xây dựng bằng Bitwise Indexing:
    
    // Đợt 1: 8 ô đầu tiên (0..7)
    uint8_t batch1_coords[8][3];
    for (int i = 0; i < 8; i++) 
    {
        batch1_coords[i][0] = i % 3;
        batch1_coords[i][1] = (i / 3) % 3;
        batch1_coords[i][2] = i / 9;
    }

    FragAtomic b1 = mat3x3x3_get_frag_atomic_vec(m, batch1_coords);
    if (callback) callback(&b1, user_data);
    mat3x3x3_set_atomic_avx8(m, &b1);
    total_mask |= b1.batch_mask;

    // Đợt 2: 8 ô tiếp theo (8..15)
    uint8_t batch2_coords[8][3];
    for (int i = 0; i < 8; i++) 
    {
        int idx = i + 8;
        batch2_coords[i][0] = idx % 3;
        batch2_coords[i][1] = (idx / 3) % 3;
        batch2_coords[i][2] = idx / 9;
    }

    FragAtomic b2 = mat3x3x3_get_frag_atomic_vec(m, batch2_coords);
    if (callback) callback(&b2, user_data);
    mat3x3x3_set_atomic_avx8(m, &b2);
    total_mask |= b2.batch_mask;

    // Đợt 3: 8 ô tiếp theo (16..23)
    uint8_t batch3_coords[8][3];

    for (int i = 0; i < 8; i++) 
    {
        int idx = i + 16;
        batch3_coords[i][0] = idx % 3;
        batch3_coords[i][1] = (idx / 3) % 3;
        batch3_coords[i][2] = idx / 9;
    }

    FragAtomic b3 = mat3x3x3_get_frag_atomic_vec(m, batch3_coords);
    if (callback) callback(&b3, user_data);
    mat3x3x3_set_atomic_avx8(m, &b3);
    total_mask |= b3.batch_mask;

    // 3 ô cuối cùng (24..26)
    for (uint8_t i = 24; i < 27; i++) 
    {
        uint8_t x = i % 3;
        uint8_t y = (i / 3) % 3;
        uint8_t z = i / 9;
        FragX1 cell = mat3x3x3_get_frag_atomic(m, x, y, z);
        cell.val *= 2.0f; // Thao tác mẫu
        mat3x3x3_set_frag_atomic(m, cell);
        total_mask |= cell.bitmask;
    }

    if (out_mask) *out_mask = total_mask; // 0x7FFFFFF (Đủ 27 bit 1 = Đã vắt cạn 27 hạt)
}

#endif
