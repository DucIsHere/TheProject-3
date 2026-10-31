#ifndef MATRIX_3D_H
#define MATRIX_3D_H

#include <stdint.h>
#include <string.h>

// ==========================================================
// MACRO HỖ TRỢ ALIGNMENT CHO AVX2 / SSE (C11 standard stdalign.h)
// ==========================================================
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #include <stdalign.h>
    #define ALIGN32 alignas(32)
#elif defined(_MSC_VER)
    #define ALIGN32 __declspec(align(32))
#else
    #define ALIGN32 __attribute__((aligned(32)))
#endif

// ==========================================================
// 1. MA TRẬN 2x2x2 (8 elements)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[8];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0;
            float x0y1z0, x1y1z0;
            // Z = 1
            float x0y0z1, x1y0z1;
            float x0y1z1, x1y1z1;
        };
    };
} Mat2x2x2;

static inline uint8_t mat2x2x2_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 4) + (y * 2) + x;
}
static inline Mat2x2x2 mat2x2x2_zero(void) { Mat2x2x2 m = {0}; return m; }
static inline Mat2x2x2 mat2x2x2_from_array(const float* arr) {
    Mat2x2x2 m; memcpy(m.data, arr, 8 * sizeof(float)); return m;
}
static inline void mat2x2x2_set(Mat2x2x2* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat2x2x2_index(x, y, z)] = val;
}
static inline float mat2x2x2_get(const Mat2x2x2* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat2x2x2_index(x, y, z)];
}

// ==========================================================
// 2. MA TRẬN 2x3x2 (12 elements -> Padded 16)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[16];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0;
            float x0y1z0, x1y1z0;
            float x0y2z0, x1y2z0;
            // Z = 1
            float x0y0z1, x1y0z1;
            float x0y1z1, x1y1z1;
            float x0y2z1, x1y2z1;

            float _pad[4];
        };
    };
} Mat2x3x2;

static inline uint8_t mat2x3x2_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 6) + (y * 2) + x;
}
static inline Mat2x3x2 mat2x3x2_zero(void) { Mat2x3x2 m = {0}; return m; }
static inline Mat2x3x2 mat2x3x2_from_array(const float* arr) {
    Mat2x3x2 m = {0}; memcpy(m.data, arr, 12 * sizeof(float)); return m;
}
static inline void mat2x3x2_set(Mat2x3x2* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat2x3x2_index(x, y, z)] = val;
}
static inline float mat2x3x2_get(const Mat2x3x2* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat2x3x2_index(x, y, z)];
}

// ==========================================================
// 3. MA TRẬN 2x2x3 (12 elements -> Padded 16)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[16];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0;
            float x0y1z0, x1y1z0;
            // Z = 1
            float x0y0z1, x1y0z1;
            float x0y1z1, x1y1z1;
            // Z = 2
            float x0y0z2, x1y0z2;
            float x0y1z2, x1y1z2;

            float _pad[4];
        };
    };
} Mat2x2x3;

static inline uint8_t mat2x2x3_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 4) + (y * 2) + x;
}
static inline Mat2x2x3 mat2x2x3_zero(void) { Mat2x2x3 m = {0}; return m; }
static inline Mat2x2x3 mat2x2x3_from_array(const float* arr) {
    Mat2x2x3 m = {0}; memcpy(m.data, arr, 12 * sizeof(float)); return m;
}
static inline void mat2x2x3_set(Mat2x2x3* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat2x2x3_index(x, y, z)] = val;
}
static inline float mat2x2x3_get(const Mat2x2x3* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat2x2x3_index(x, y, z)];
}

// ==========================================================
// 4. MA TRẬN 2x2x1 (4 elements -> Padded 8)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[8];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0;
            float x0y1z0, x1y1z0;

            float _pad[4];
        };
    };
} Mat2x2x1;

static inline uint8_t mat2x2x1_index(uint8_t x, uint8_t y, uint8_t z) {
    (void)z;
    return (y * 2) + x;
}
static inline Mat2x2x1 mat2x2x1_zero(void) { Mat2x2x1 m = {0}; return m; }
static inline Mat2x2x1 mat2x2x1_from_array(const float* arr) {
    Mat2x2x1 m = {0}; memcpy(m.data, arr, 4 * sizeof(float)); return m;
}
static inline void mat2x2x1_set(Mat2x2x1* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat2x2x1_index(x, y, z)] = val;
}
static inline float mat2x2x1_get(const Mat2x2x1* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat2x2x1_index(x, y, z)];
}

// ==========================================================
// 5. MA TRẬN 2x1x2 (4 elements -> Padded 8)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[8];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0;
            // Z = 1
            float x0y0z1, x1y0z1;

            float _pad[4];
        };
    };
} Mat2x1x2;

static inline uint8_t mat2x1x2_index(uint8_t x, uint8_t y, uint8_t z) {
    (void)y;
    return (z * 2) + x;
}
static inline Mat2x1x2 mat2x1x2_zero(void) { Mat2x1x2 m = {0}; return m; }
static inline Mat2x1x2 mat2x1x2_from_array(const float* arr) {
    Mat2x1x2 m = {0}; memcpy(m.data, arr, 4 * sizeof(float)); return m;
}
static inline void mat2x1x2_set(Mat2x1x2* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat2x1x2_index(x, y, z)] = val;
}
static inline float mat2x1x2_get(const Mat2x1x2* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat2x1x2_index(x, y, z)];
}

// ==========================================================
// 6. MA TRẬN 3x1x3 (9 elements -> Padded 16)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[16];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0, x2y0z0;
            // Z = 1
            float x0y0z1, x1y0z1, x2y0z1;
            // Z = 2
            float x0y0z2, x1y0z2, x2y0z2;

            float _pad[7];
        };
    };
} Mat3x1x3;

static inline uint8_t mat3x1x3_index(uint8_t x, uint8_t y, uint8_t z) {
    (void)y;
    return (z * 3) + x;
}
static inline Mat3x1x3 mat3x1x3_zero(void) { Mat3x1x3 m = {0}; return m; }
static inline Mat3x1x3 mat3x1x3_from_array(const float* arr) {
    Mat3x1x3 m = {0}; memcpy(m.data, arr, 9 * sizeof(float)); return m;
}
static inline void mat3x1x3_set(Mat3x1x3* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat3x1x3_index(x, y, z)] = val;
}
static inline float mat3x1x3_get(const Mat3x1x3* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat3x1x3_index(x, y, z)];
}

// ==========================================================
// 7. MA TRẬN 3x2x3 (18 elements -> Padded 32)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[32];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0, x2y0z0;
            float x0y1z0, x1y1z0, x2y1z0;
            // Z = 1
            float x0y0z1, x1y0z1, x2y0z1;
            float x0y1z1, x1y1z1, x2y1z1;
            // Z = 2
            float x0y0z2, x1y0z2, x2y0z2;
            float x0y1z2, x1y1z2, x2y1z2;

            float _pad[14];
        };
    };
} Mat3x2x3;

static inline uint8_t mat3x2x3_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 6) + (y * 3) + x;
}
static inline Mat3x2x3 mat3x2x3_zero(void) { Mat3x2x3 m = {0}; return m; }
static inline Mat3x2x3 mat3x2x3_from_array(const float* arr) {
    Mat3x2x3 m = {0}; memcpy(m.data, arr, 18 * sizeof(float)); return m;
}
static inline void mat3x2x3_set(Mat3x2x3* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat3x2x3_index(x, y, z)] = val;
}
static inline float mat3x2x3_get(const Mat3x2x3* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat3x2x3_index(x, y, z)];
}

// ==========================================================
// 8. MA TRẬN 3x3x3 (27 elements -> Padded 32)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[32];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0, x2y0z0;
            float x0y1z0, x1y1z0, x2y1z0;
            float x0y2z0, x1y2z0, x2y2z0;
            // Z = 1
            float x0y0z1, x1y0z1, x2y0z1;
            float x0y1z1, x1y1z1, x2y1z1;
            float x0y2z1, x1y2z1, x2y2z1;
            // Z = 2
            float x0y0z2, x1y0z2, x2y0z2;
            float x0y1z2, x1y1z2, x2y1z2;
            float x0y2z2, x1y2z2, x2y2z2;

            float _pad[5];
        };
    };
} Mat3x3x3;

static inline uint8_t mat3x3x3_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 9) + (y * 3) + x;
}
static inline Mat3x3x3 mat3x3x3_zero(void) { Mat3x3x3 m = {0}; return m; }
static inline Mat3x3x3 mat3x3x3_from_array(const float* arr) {
    Mat3x3x3 m = {0}; memcpy(m.data, arr, 27 * sizeof(float)); return m;
}
static inline void mat3x3x3_set(Mat3x3x3* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat3x3x3_index(x, y, z)] = val;
}
static inline float mat3x3x3_get(const Mat3x3x3* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat3x3x3_index(x, y, z)];
}

// ==========================================================
// 9. MA TRẬN 3x3x1 (9 elements -> Padded 16)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[16];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0, x2y0z0;
            float x0y1z0, x1y1z0, x2y1z0;
            float x0y2z0, x1y2z0, x2y2z0;

            float _pad[7];
        };
    };
} Mat3x3x1;

static inline uint8_t mat3x3x1_index(uint8_t x, uint8_t y, uint8_t z) {
    (void)z;
    return (y * 3) + x;
}
static inline Mat3x3x1 mat3x3x1_zero(void) { Mat3x3x1 m = {0}; return m; }
static inline Mat3x3x1 mat3x3x1_from_array(const float* arr) {
    Mat3x3x1 m = {0}; memcpy(m.data, arr, 9 * sizeof(float)); return m;
}
static inline void mat3x3x1_set(Mat3x3x1* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat3x3x1_index(x, y, z)] = val;
}
static inline float mat3x3x1_get(const Mat3x3x1* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat3x3x1_index(x, y, z)];
}

// ==========================================================
// 10. MA TRẬN 3x3x2 (18 elements -> Padded 32)
// ==========================================================
typedef struct ALIGN32 {
    union {
        float data[32];
        struct {
            // Z = 0
            float x0y0z0, x1y0z0, x2y0z0;
            float x0y1z0, x1y1z0, x2y1z0;
            float x0y2z0, x1y2z0, x2y2z0;
            // Z = 1
            float x0y0z1, x1y0z1, x2y0z1;
            float x0y1z1, x1y1z1, x2y1z1;
            float x0y2z1, x1y2z1, x2y2z1;

            float _pad[14];
        };
    };
} Mat3x3x2;

static inline uint8_t mat3x3x2_index(uint8_t x, uint8_t y, uint8_t z) {
    return (z * 9) + (y * 3) + x;
}
static inline Mat3x3x2 mat3x3x2_zero(void) { Mat3x3x2 m = {0}; return m; }
static inline Mat3x3x2 mat3x3x2_from_array(const float* arr) {
    Mat3x3x2 m = {0}; memcpy(m.data, arr, 18 * sizeof(float)); return m;
}
static inline void mat3x3x2_set(Mat3x3x2* m, uint8_t x, uint8_t y, uint8_t z, float val) {
    m->data[mat3x3x2_index(x, y, z)] = val;
}
static inline float mat3x3x2_get(const Mat3x3x2* m, uint8_t x, uint8_t y, uint8_t z) {
    return m->data[mat3x3x2_index(x, y, z)];
}

#endif // MATRIX_H