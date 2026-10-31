#ifndef MATRIX_MINUS_H
#define MATRIX_MINUS_H

#include "Matrix.h"

// ==========================================================
// 1. PHÉP TOÁN KHẤU TRỪ VÀ ĐẢO NGƯỢC MA TRẬN TĨNH (STACK)
// ==========================================================

/**
 * Trừ hai ma trận 3x3 tĩnh: res = A - B
 */
static inline Mat3x3 mat3x3_sub(const Mat3x3* A, const Mat3x3* B) 
{
    Mat3x3 res;
    for (int i = 0; i < 9; i++) res.m[i] = A->m[i] - B->m[i];
    return res;
}

/**
 * Trừ hai ma trận 2x2 tĩnh: res = A - B
 */
static inline Mat2x2 mat2x2_sub(const Mat2x2* A, const Mat2x2* B) 
{
    Mat2x2 res;
    for (int i = 0; i < 4; i++) res.m[i] = A->m[i] - B->m[i];
    return res;
}

/**
 * Trừ một hằng số khỏi ma trận 3x3: res = A - scalar
 */
static inline Mat3x3 mat3x3_sub_scalar(const Mat3x3* A, float scalar) 
{
    Mat3x3 res;
    for (int i = 0; i < 9; i++) res.m[i] = A->m[i] - scalar;
    return res;
}

/**
 * Đảo dấu toàn bộ ma trận 3x3 (Negate): res = -A
 */
static inline Mat3x3 mat3x3_negate(const Mat3x3* A) 
{
    Mat3x3 res;
    for (int i = 0; i < 9; i++) res.m[i] = -A->m[i];
    return res;
}


// ==========================================================
// 2. PHÉP TOÁN TÍNH PHÂN KỲ & ĐỘ LỆCH MẢNG PHẲNG (FloatMatrix View)
// ==========================================================

/**
 * Tính toán độ chênh lệch tuyệt đối giữa 2 mảng địa hình: dest = abs(dest - src)
 * Thường dùng để đo lượng thay đổi địa hình sau một chu kỳ xói mòn (Error/Delta map).
 */
static inline void mat_abs_diff(FloatMatrix* dest, const FloatMatrix* src) 
{
    int total_cells = dest->width * dest->height;

    for (int i = 0; i < total_cells; i++) 
    {
        dest->data[i] = fabsf(dest->data[i] - src->data[i]);
    }
}

/**
 * Phép toán trừ tích lũy Fused Multiply-Subtract (MSB): dest = dest - (src * scalar)
 * Ứng dụng trực tiếp: Bào mòn độ cao địa hình dựa trên ma trận tốc độ dòng chảy của nước.
 */
static inline void mat_msb(FloatMatrix* dest, const FloatMatrix* src, float scalar) 
{
    int total_cells = dest->width * dest->height;

    for (int i = 0; i < total_cells; i++) 
    {
        dest->data[i] -= src->data[i] * scalar;
    }
}

/**
 * Khấu trừ sàn độ cao (Threshold Subtraction): Nếu giá trị > min_val thì trừ đi, ngược lại gán bằng 0
 * Ứng dụng: Mô phỏng sự bốc hơi của nước bám trên bề mặt đá hoặc độ thấm hút của đất qua thời gian.
 */
static inline void mat_evaporate(FloatMatrix* m, float rate) 
{
    int total_cells = m->width * m->height;

    for (int i = 0; i < total_cells; i++) 
    {
        m->data[i] = (m->data[i] > rate) ? (m->data[i] - rate) : 0.0f;
    }
}

/**
 * Tính toán toán tử Laplace rời rạc cơ bản (Độ cong địa hình/Độ lệch áp suất):
 * dest[x,y] = 4 * src[x,y] - (src[x+1,y] + src[x-1,y] + src[x,y+1] + src[x,y-1])
 * Hàm này dùng toán tử trừ để tìm ra các điểm lồi lõm cực hạn trên bản đồ.
 */
static inline void mat_laplacian_kernel(FloatMatrix* dest, const FloatMatrix* src) 
{
    int w = src->width;
    int h = src->height;
    
    // Quét lõi, bỏ qua biên để tránh lỗi tràn bộ nhớ
    for (int y = 1; y < h - 1; y++) 
    {
        for (int x = 1; x < w - 1; x++) 
        {
            float center = src->data[y * w + x];
            float left   = src->data[y * w + (x - 1)];
            float right  = src->data[y * w + (x + 1)];
            float up     = src->data[(y - 1) * w + x];
            float down   = src->data[(y + 1) * w + x];
            
            dest->data[y * w + x] = (4.0f * center) - (left + right + up + down);
        }
    }
}

#endif // MATRIX_MINUS_H