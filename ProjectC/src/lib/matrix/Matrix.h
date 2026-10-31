#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct Mat2x2 {
    float m[4];
};

typedef struct Mat2x1 {
    float m[2];
};

typedef struct Mat2x3 {
    float m[6];
};

typedef struct Mat3x1 {
    float m[3];
};

typedef struct Mat3x2 {
    float m[6];
};

typedef struct Mat3x3 {
    float m[9];
};

static inline Mat2x1 mat2x1_set(float m0, float m1) {
    Mat2x1 mat;
    mat.m[0] = m0; mat.m[1] = m1;
    return mat;
}

static inline Mat2x2 mat2x2_set(float m00, float m01, float m10, float m11)
{
    Mat2x2 mat;
    mat.m[0] = m00; mat.m[1] = m01;
    mat.m[2] = m10; mat.m[3] = m11;
    return mat;
}

static inline Mat2x3 mat2x3_set(float m00, float m01, float m10, float m11, float m21, float m22)
{
    Mat2x3 mat;
    mat.m[0] = m00; mat.m[1] = m01;
    mat.m[2] = m10; mat.m[3] = m11;
    mat.m[4] = m21; mat.m[5] = m22;
    return mat;
}

static inline Mat3x1 mat3x1_set(float m00, float m01, float m10)
{
    Mat3x1 mat;
    mat.m[0] = m00; mat.m[1] = m01; mat.m[2] = m10;
    return mat;
}

static inline Mat3x2 mat3x2_set(float m00, float m01, float m10, float m11, float m21, float m22)
{
    Mat3x2 mat;
    mat.m[0] = m00; mat.m[1] = m01; mat.m[2] = m10;
    mat.m[3] = m11; mat.m[4] = m21; mat.m[5] = m22;
    return mat;
}

static inline Mat3x3 mat3x3_set(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
    Mat3x3 mat;
    mat.m[0] = m00; mat.m[1] = m01; mat.m[2] = m02;
    mat.m[3] = m10; mat.m[4] = m11; mat.m[5] = m12;
    mat.m[6] = m20; mat.m[7] = m21; mat.m[8] = m22;
    return mat;
}
