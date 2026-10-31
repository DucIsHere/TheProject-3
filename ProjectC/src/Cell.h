#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdalign.h>

typedef struct Cell Cell;

struct alignas(8) Cell {
    float* height;
    float* loacl_erosion;
    float* local_erosion2;
    float* sediment;
    float* sediment2;
    float* gradient;
    float* regionMoisture;
    float* regionTemperature;
    float* continentId;
    float* continentNoise;
    float* continentEdge;
    float* continentTalness;
    float* terrainRegionId;
    float* terrainRegionEdge;
    int64_t terrainRegionCenter;
    float* biomeRegionId;
    float* biomeRegionEdge;
    float* macroBiomeId;

    float* biomVariant;
    float* riverDistance;
    float* waterLevel;
    float* riverBank;

    int32_t coast_feature;

    float* riverWidth;
    const float FAR;
    int32_t continentX;
    int32_t continentZ;
    bool erosionMask;
    float terrainMASK;

    float* mountainChainAlpha;
    float* erosion;
    float* weirdness;
    float* temperature;
    float* moisture;
};

void cell_init(Cell* cell);
void cell_copy(Cell* dst, Cell* src);
void cell_reset(Cell* cell);