#ifndef MATRIX_2D_FRAGMENT_H
#define MATRIX_2D_FRAGMENT_H

#include <stdint.h>
#include <string.h>
#include <immintriìnde

#include "Matrix.h"

// Alignment 32-Byte bắt buộc cho AVX/AVX2
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #include <stdalign.h>
    #define ALIGN32 alignas(32)
#elif defined(_MSC_VER)
    #define ALIGN32 __declspec(align(32))
#else
    #define ALIGN32 __attribute__((aligned(32)))
#endif

// ==========================================================
// 1. CẤU TRÚC MẢNH 2D XZ DẸT DÙNG BỘ ĐỆM SIMD/AVX
// ==========================================================

// Mảnh 2D 3x3 (Cho Mat3x3x3 hoặc Mat3x2x3)
// 9 float + 3 float pad = 12 float (48 bytes) -> Vừa tròn 3 thanh ghi SSE __m128 hoặc 1.5 AVX2
typedef struct ALIGN32 {
    float xz[12];      // Mảng phẳng dẹt chứa trọn vẹn mặt phẳng XZ tại tầng Y
    uint8_t y;         // Tọa độ tầng Y (Topology Tag)
    uint32_t bit_mask; // Bitmask đại diện tầng: (1U << y)
} FragXZ_3x3_SIMD;

// Mảnh 2D 2x2 (Cho Mat2x2x2 hoặc Mat2x3x2)
// 4 float = 16 bytes -> Vừa tròn 1 thanh ghi SSE __m128
typedef struct ALIGN32 {
    float xz[4];       // Mảng phẳng dẹt chứa trọn vẹn mặt phẳng XZ 2x2
    uint8_t y;         // Tọa độ tầng Y
    uint32_t bit_mask; // Bitmask đại diện tầng: (1U << y)
} FragXZ_2x2_SIMD;


// ==========================================================
// 2. HELPER: BITMASK TẦNG Y
// ==========================================================
static inline uint32_t frag_xz_calc_bitmask(uint8_t y) {
    return (1U << y);
}


// ==========================================================
// 3. TRÍCH XUẤT MẢNH 2D XZ BẰNG SIMD (HÚT CẠN TẦNG Y)
// ==========================================================

// --- Mat3x3x3 ---
static inline FragXZ_3x3_SIMD mat3x3x3_get_frag_xz_simd(const Mat3x3x3* m, uint8_t y) {
    FragXZ_3x3_SIMD frag;
    frag.y = y;
    frag.bit_mask = frag_xz_calc_bitmask(y);

    // Hút cạn 3 dải Z của tầng Y bằng SIMD Load
    for (uint8_t z = 0; z < 3; z++) {
        uint8_t idx = mat3x3x3_index(0, y, z);
        __m128 v = _mm_loadu_ps(&m->data[idx]);
        _mm_store_ps(&frag.xz[z * 3], v); // Lưu thẳng vào mảng dẹt đã align 32-byte
    }

    // Clear padding cho sạch thanh ghi SIMD
    frag.xz[9] = frag.xz[10] = frag.xz[11] = 0.0f;
    return frag;
}

// --- Mat2x3x2 ---
static inline FragXZ_2x2_SIMD mat2x3x2_get_frag_xz_simd(const Mat2x3x2* m, uint8_t y) {
    FragXZ_2x2_SIMD frag;
    frag.y = y;
    frag.bit_mask = frag_xz_calc_bitmask(y);

    uint8_t idx0 = mat2x3x2_index(0, y, 0);
    uint8_t idx1 = mat2x3x2_index(0, y, 1);

    // Gom 4 phần tử (Z=0: X0,X1 và Z=1: X0,X1) dồn dẹt vào 1 thanh ghi SSE
    frag.xz[0] = m->data[idx0];
    frag.xz[1] = m->data[idx0 + 1];
    frag.xz[2] = m->data[idx1];
    frag.xz[3] = m->data[idx1 + 1];

    return frag;
}


// ==========================================================
// 4. RECONSTRUCTION: ĐỔ MẢNH 2D ĐÃ XỬ LÝ TRỞ LẠI MA TRẬN 3D
// ==========================================================

static inline void mat3x3x3_set_frag_xz_simd(Mat3x3x3* m, const FragXZ_3x3_SIMD* frag) {
    for (uint8_t z = 0; z < 3; z++) {
        uint8_t idx = mat3x3x3_index(0, frag->y, z);
        // Ghi lại RAM 3D theo đúng vị trí hình học ban đầu
        m->data[idx]     = frag->xz[z * 3 + 0];
        m->data[idx + 1] = frag->xz[z * 3 + 1];
        m->data[idx + 2] = frag->xz[z * 3 + 2];
    }
}

static inline void mat2x3x2_set_frag_xz_simd(Mat2x3x2* m, const FragXZ_2x2_SIMD* frag) {
    uint8_t idx0 = mat2x3x2_index(0, frag->y, 0);
    uint8_t idx1 = mat2x3x2_index(0, frag->y, 1);

    m->data[idx0]     = frag->xz[0];
    m->data[idx0 + 1] = frag->xz[1];
    m->data[idx1]     = frag->xz[2];
    m->data[idx1 + 1] = frag->xz[3];
}


// ==========================================================
// 5. AUTO-ITERATOR: TỰ DỊCH Y TỪ 0..2 & TÁI CẤU TRÚC THỜI GIAN THỰC
// ==========================================================

typedef void (*FragXZ_3x3_SIMD_Callback)(FragXZ_3x3_SIMD* frag, void* user_data);

static inline void mat3x3x3_process_all_xz_simd(Mat3x3x3* m, FragXZ_3x3_SIMD_Callback callback, void* user_data, uint32_t* out_mask) {
    uint32_t mask = 0;

    // Tự động dịch qua 3 tầng Y (Y=0, Y=1, Y=2)
    for (uint8_t y = 0; y < 3; y++) {
        // 1. Extract 2D Slice
        FragXZ_3x3_SIMD frag = mat3x3x3_get_frag_xz_simd(m, y);

        // 2. Parallel Processing (Thao tác SIMD trên mảnh frag.xz)
        if (callback) {
            callback(&frag, user_data);
        }

        // 3. Topology Reconstruction
        mat3x3x3_set_frag_xz_simd(m, &frag);

        // 4. Update Bitmask Status (0x1 -> 0x3 -> 0x7)
        mask |= frag.bit_mask;
    }

    if (out_mask) *out_mask = mask; // Đạt 0x7 là hoàn thành 100% tái cấu trúc 3D
}

#endif // MATRIX_2D_FRAGMENT_H

#endif