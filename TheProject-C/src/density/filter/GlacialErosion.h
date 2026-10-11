#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include ".../density/Modifier.h"
#include ".../cell/Cell.h"
#include ".../include/math/Interpolation.h"
#include ".../include/math/extern/InterporlationFast.h"
#include ".../include/math/NoiseUtil.h"
#include ".../thread/threads/pool/ThreadPool.h"

typedef struct TerrainPos TerrainPos;
typedef struct Brushes Brushes;
typedef struct Glacial Glacial;
typedef struct GlacialContext GlacialContext;

struct Glacial {
    int32_t mapSize;
    int32_t totalCells;
    Modifier modifier;

    float RHO_ICE;
    float GRAVITY;
    float RHO_MANTLE;
    float TO_KELVIN;
    float SPECIFIC_HEAT_ICE;

    // Các tham số vật lý băng hà
    float slidingCoefficient;
    float erosionConstant;
    float viscosityFactor;
    float kinWaveSpeed;
    float elaHeight;
    float massBalanceGradient;
    float flexuralRigidity;
    float thermalDiffusivity;
    float degreeDayFactor;
    float tillTransportRate;
    float effectivePressureCoeff;
    float cryoDamageRate;

    // Flattened Brushes (CSR Matrix)
    int32_t* brushOffsets;
    int32_t* brushSizes;
    int32_t* brushIndices;
    float*   brushWeights;
    TerrainPos* positions;
    int32_t totalCells;
};

// Cấu trúc nội suy địa hình Bilinear
struct TerrainPos {
    float height;
    float gradientsY;
    float gradientsX;
    float _padding[3];
};

struct GlacialContext {
    Thrd* pool;
    Glacial* conf;
    Cell* cells;
    int32_t width;
    int32_t height;
    int32_t totalCells;
    int32_t lengthChunk;
    int32_t borderChunk;
    int32_t chunkX;
    int32_t chunkZ;
    uint64_t seed;
    int32_t iterations;
};

struct __attribute__((packed)) Brushes {
    int32_t* offsets;   // Kích thước: size * size
    int32_t* sizes;     // Kích thước: size * size
    int32_t* indices;   // Kích thước: Tổng numEntries toàn bản đồ
    float*   weights;   // Kích thước: Tổng numEntries toàn bản đồ
    int32_t  total_entries;
};

Glacial* c23_create_glacial_config(
    Glacial* conf,
    int32_t mapSize,
    int32_t totalCells,
    Modifier modifier,

    float RHO_ICE,
    float GRAVITY,
    float RHO_MANTLE,
    float TO_KELVIN,
    float SPECIFIC_HEAT_ICE,

    // Các tham số vật lý băng hà
    float slidingCoefficient,
    float erosionConstant,
    float viscosityFactor,
    float kinWaveSpeed,
    float elaHeight,
    float massBalanceGradient,
    float flexuralRigidity,
    float thermalDiffusivity,
    float degreeDayFactor,
    float tillTransportRate,
    float effectivePressureCoeff,
    float cryoDamageRate,

    // Flattened Brushes (CSR Matrix)
    int32_t* brushOffsets,
    int32_t* brushSizes,
    int32_t* brushIndices,
    float* brushWeights,
    int32_t iterations
);

void c23_free_glacial_config(GlacialConfig* config);

void c23_run_glacial_simulation(
    Glacial* config,
    Thrd* pool,
    Cell* cells,
    int32_t width,
    int32_t height,
    int32_t totalCells,
    int32_t lengthChunk,
    int32_t borderChunk,
    int32_t chunkX,
    int32_t chunkZ,
    uint64_t seed,
    int32_t iterations
);

Glacial* c23_glacial_conf(Glacial* conf);
void c23_glacial_free(Glacial* cf);