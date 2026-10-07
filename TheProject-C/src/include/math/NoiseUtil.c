#ifndef NOISE_UTIL_C
#define NOISE_UTIL_C

#include "NoiseUtil.h"

#include <math.h>

// 1. ĐỊNH NGHĨA VÙNG NHỚ THỰC TẾ CHO CÁC MẢNG PHẲNG TĨNH
Vec2f CELL_2D[256];
Vec2f GRAD_2D[8];         // Đồng bộ đúng kích thước mảng Perlin cơ bản từ Java
Vec2f GRAD_2D_24[32];     // Mảng 32 hướng nâng cao cho hàm coord2D_24

/**
 * HÀM TIẾP TẾ TOÀN DIỆN: Nhận các dải dữ liệu duỗi phẳng float[] từ Java qua Panama FFI
 * Cấu trúc mảng phẳng truyền xuống: [x0, y0, x1, y1, x2, y2...]
 */
void bind_native_noise_constants(
    const float* java_cell_2d, 
    const float* java_grad_2d,
    const float* java_grad_2d_24
) {
    // 2. KHỞI TẠO BẢNG SIN LƯỢNG TỬ (KHỚP ĐẾT LOGIC KHỞI TẠO TĨNH BÊN JAVA)
    // Bước 1: Tạo dải Sin gốc mượt mà
    for (int i = 0; i < SIN_COUNT; ++i) 
    {
        // Sử dụng hằng số góc PI2 = 6.2831855f từ file .h
        SIN[i] = (float)sin(((double)i + 0.5) / SIN_COUNT * PI2);
    }

    // Bước 2: Ép góc vuông (90, 180, 270) về giá trị chuẩn để triệt tiêu sai số float
    float degToIndex = SIN_COUNT / 360.0f;
    for (int i = 0; i < 360; i += 90) 
    {
        int index = (int)(i * degToIndex) & SIN_MASK;
        SIN[index] = (float)sin(i * 3.141592653589793 / 180.0);
    }

    // 3. ĐỔ DỮ LIỆU TỪ JAVA XUỐNG CÁC MẢNG STRUCT PHẲNG DƯỚI C

    // Nạp mảng CELL_2D (256 phần tử)
    for (int i = 0; i < 256; i++)
    {
        CELL_2D[i].x = java_cell_2d[i * 2];
        CELL_2D[i].y = java_cell_2d[i * 2 + 1];
    }

    // Nạp mảng GRAD_2D (8 phần tử cơ bản)
    for (int i = 0; i < 8; i++)
    {
        GRAD_2D[i].x = java_grad_2d[i * 2];
        GRAD_2D[i].y = java_grad_2d[i * 2 + 1];
    }

    // Nạp mảng GRAD_2D_24 (32 phần tử nâng cao)
    for (int i = 0; i < 32; i++)
    {
        GRAD_2D_24[i].x = java_grad_2d_24[i * 2];
        GRAD_2D_24[i].y = java_grad_2d_24[i * 2 + 1];
    }
}

#endif
