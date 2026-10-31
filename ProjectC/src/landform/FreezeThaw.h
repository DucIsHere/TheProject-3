#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#include "include\NoiseUtil.h"
#include "include\Interpolation.h"

typedef struct FreezeThaw FreezeThaw;
typedef struct Context Context;
typedef struct TerrainPos TerrainPos;
typedef struct Brush Brush;

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
    int32_t* brushOffset;
    Modifier modifier;
};

struct Context {
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

    float* damageMap;
    float* moistureMap;
};
