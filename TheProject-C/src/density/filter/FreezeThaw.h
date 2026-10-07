#pragma once

#include <float.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include ".../thread/threads/pool/ThreadPool.h"
#include ".../cell/Cell.h"
#include ".../include/math/NoiseUtil.h"
#include ".../include/util/FastRandom.h"

typedef struct FreezeThaw FreezeThaw;
typedef struct TerrainPos TerrainPos;
typedef struct FreezeThawTaskContext FreezeThawTaskContext;
typedef struct TerrainPos TerrainPos;

struct FreezeThaw {
    // 1. Physical Constants
    float pW;                // Density of water: 1000.0 kg/m^3
    float pI;                // Density of ice: 987.0 kg/m^3
    float Lf;                // Latent heat of fusion: 334000.0 J/kg
    float cW;                // Specific heat water: 4184.0 J/kg*K
    float cI;                // Specific heat ice: 2108.9 J/kg*K
    float kW;                // Thermal conductivity water: 0.56 W/m*K
    float kI;                // Thermal conductivity ice: 2.22 W/m*K
    float To;                // Reference temp: 273.15 K
    float freezeMaxDepth;    // Max freeze depth: 3.0 m

    // 2. Config Parameters
    float porosity;          // Porosity
    float tensileStrength;   // Tensile strength
    float criticalSaturation;// Critical saturation
    float softeningFactor;  // Softening factor
    float clapeyronFactor;   // 1.11f
    float freezeThawCycles;  // Number of cycles
    float mExponent;         // 2.0f
    float nExponent;         // 1.2f

    float breakAmount;       // Break amount
    float zAmountDepth;      // Z amount depth
    float dDampingDepth;     // D damping depth
    float snowLine;          // 0.5f
    float sedimentCapacity;  // 0.02f

    // 3. Brush Memory Pointers (Flattened 1D arrays từ Java)
    TerrainPos* positions;
    int32_t mapSize;
    int32_t totalCells;
    int32_t* brushIndices;   // Flattened array erosionBrushIndices
    float* brushWeights;     // Flattened array erosionBrushWeights
    int32_t* brushSizes;     // Số lượng phần tử brush của từng Cell
};

struct FreezeThawTaskContext {
    Thrd* pool;
    FreezeThaw* conf;
    Cell* cells;
    float* scratchDamageMap;
    float* scratchMoistureMap;
    int32_t regionX;
    int32_t regionZ;
    int32_t iterationPerChunks;

    int32_t blockX;
    int32_t blockZ;
    int32_t width;
    int32_t height;
    int32_t border;
};

struct TerrainPos {
    float x;
    float y;
    float z;
    float _padding[3];
};

FreezeThaw* c23_create_config(
    Thrd* pool,
    FreezeThawConfig* config,
    Cell* cells,
    float* scratchDamageMap,
    float* scratchMoistureMap,
    int32_t blockX,          // Lấy từ tile.getBlockX()
    int32_t blockZ,          // Lấy từ tile.getBlockZ()
    int32_t width,           // Lấy từ tile.getBlockSize().width()
    int32_t height,          // Lấy từ tile.getBlockSize().height()
    int32_t border,          // Lấy từ tile.getBlockSize().border()
    uint64_t seed,
    int32_t iterationsPerChunk
);

void c23_free_config(FreezeThaw* conf);