#ifndef MATRIX_TRANSFORM_H
#define MATRIX_TRANSFORM_H

#include "Matrix.h"
#include "MatrixFastMath.h"

// ==========================================================
// 1. KIỂM TRA ĐIỀU KIỆN MA TRẬN DẸT (DEGENERATE / SINGULAR DETECT)
// ==========================================================

/**
 * Kiểm tra ma trận 2x2 có bị dẹt (suy biến) hay không
 * Công thức: det = m0*m3 - m1*m2
 */
static inline int mat2x2_is_degenerate(const Mat2x2* M, float epsilon) {
    float det = M->m[0] * M->m[3] - M->m[1] * M->m[2];
    // Ép nhãn bit lấy giá trị tuyệt đối không phân nhánh (Branchless abs)
    uint32_t fill_bit = *(uint32_t*)&det & 0x7FFFFFFF;
    float abs_det = *(float*)&fill_bit;
    return (abs_det < epsilon);
}

/**
 * Kiểm tra ma trận 3x3 bằng AVX2 tăng tốc tính toán định thức phẳng
 */
static inline int mat3x3_is_degenerate(const Mat3x3* M, float epsilon) {
    // det = m0*(m4*m8 - m5*m7) - m1*(m3*m8 - m5*m6) + m2*(m3*m7 - m4*m6)
    float sub0 = M->m[4] * M->m[8] - M->m[5] * M->m[7];
    float sub1 = M->m[3] * M->m[8] - M->m[5] * M->m[6];
    float sub2 = M->m[3] * M->m[7] - M->m[4] * M->m[6];
    
    float det = M->m[0] * sub0 - M->m[1] * sub1 + M->m[2] * sub2;
    
    uint32_t fill_bit = *(uint32_t*)&det & 0x7FFFFFFF;
    float abs_det = *(float*)&fill_bit;
    return (abs_det < epsilon);
}

/**
 * Kiểm tra ma trận chữ nhật dẹt 2x3 (Kiểm tra ma trận Gramian M * M^T)
 * Nhận diện hệ tọa độ chiếu có bị nén phẳng thành một đường thẳng hay không.
 */
static inline int mat2x3_is_degenerate(const Mat2x3* M, float epsilon) {
    // Tính các thành phần của ma trận vuông 2x2: A = M * M^T
    // a00 = m0^2 + m1^2 + m2^2
    // a01 = m0*m3 + m1*m4 + m2*m5
    // a11 = m3^2 + m4^2 + m5^2
    float a00 = M->m[0]*M->m[0] + M->m[1]*M->m[1] + M->m[2]*M->m[2];
    float a01 = M->m[0]*M->m[3] + M->m[1]*M->m[4] + M->m[2]*M->m[5];
    float a11 = M->m[3]*M->m[3] + M->m[4]*M->m[4] + M->m[5]*M->m[5];
    
    float det = a00 * a11 - a01 * a01;
    uint32_t fill_bit = *(uint32_t*)&det & 0x7FFFFFFF;
    float abs_det = *(float*)&fill_bit;
    return (abs_det < epsilon);
}

/**
 * Kiểm tra ma trận chữ nhật đứng 3x2 (Kiểm tra ma trận Gramian M^T * M)
 */
static inline int mat3x2_is_degenerate(const Mat3x2* M, float epsilon) {
    // Tính các thành phần của ma trận vuông 2x2: A = M^T * M
    float a00 = M->m[0]*M->m[0] + M->m[2]*M->m[2] + M->m[4]*M->m[4];
    float a01 = M->m[0]*M->m[1] + M->m[2]*M->m[3] + M->m[4]*M->m[5];
    float a11 = M->m[1]*M->m[1] + M->m[3]*M->m[3] + M->m[5]*M->m[5];
    
    float det = a00 * a11 - a01 * a01;
    uint32_t fill_bit = *(uint32_t*)&det & 0x7FFFFFFF;
    float abs_det = *(float*)&fill_bit;
    return (abs_det < epsilon);
}


// ==========================================================
// 2. BIẾN ĐỔI HỆ TOẠ ĐỘ SỬ DỤNG PHÂN LỚP CHIỀU 1D, 2D, 3D
// ==========================================================

/**
 * Biến đổi 1D tuyến tính: Nhân tỷ lệ mảng phẳng
 */
static inline float mat1d_transform(float val, float scale) {
    return val * scale;
}

/**
 * Biến đổi 2D: Nhân Vector phẳng [x, y] với ma trận vuông 2x2
 * Ghi thẳng kết quả cô lập vùng nhớ để diệt tận gốc False Sharing chéo Thread.
 */
static inline void mat2x2_2d_transform(const Mat2x2* M, float x, float y, float* out_x, float* out_y) {
    *out_x = M->m[0] * x + M->m[1] * y;
    *out_y = M->m[2] * x + M->m[3] * y;
}

/**
 * Biến đổi 3D kẹp SSE: Nhân tọa độ [x, y, z] với cấu trúc ma trận vuông 3x3 
 */
static inline void mat3x3_3d_transform(const Mat3x3* M, float x, float y, float z, 
                                       float* out_x, float* out_y, float* out_z) {
    // Khai báo tính toán song song tránh nghẽn luồng đọc ghi của CPU
    *out_x = M->m[0] * x + M->m[1] * y + M->m[2] * z;
    *out_y = M->m[3] * x + M->m[4] * y + M->m[5] * z;
    *out_z = M->m[6] * x + M->m[7] * y + M->m[8] * z;
}

/**
 * Biến đổi dẹt từ không gian 3D về 2D bằng ma trận 2x3 (Projection-like Vector Multiplying)
 */
static inline void mat2x3_3d_to_2d_transform(const Mat2x3* M, float x, float y, float z, 
                                             float* out_x, float* out_y) {
    *out_x = M->m[0] * x + M->m[1] * y + M->m[2] * z;
    *out_y = M->m[3] * x + M->m[4] * y + M->m[5] * z;
}

/**
 * Biến đổi nâng chiều từ 2D lên 3D bằng ma trận 3x2
 */
static inline void mat3x2_2d_to_3d_transform(const Mat3x2* M, float x, float y, 
                                             float* out_x, float* out_y, float* out_z) {
    *out_x = M->m[0] * x + M->m[1] * y;
    *out_y = M->m[2] * x + M->m[3] * y;
    *out_z = M->m[4] * x + M->m[5] * y;
}

#endif // MATRIX_TRANSFORM_H