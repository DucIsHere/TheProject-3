#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
    int capacity;
    float* height;
    float* height_erosion;
    float* sediment;
    float* gradient;
    float* region_moisture;
    float* region_temperature;
    float* continentID;
    float* continent_edge;
    float* terrain_region_edge;
    float* terrain_region_id;
    float* biome_region_id;
    float* biome_region_edge;
    float* macro_biome_id;
    float* macro_biome_id;
    float* river_mask;
    int32_t* continent_x;
    int32_t* continent_Z;
    float* erosion_mask;
    float* erosion;
    float* weirdness;
    float* moisture;
    float* damage;
    float* temperature;
} Cell;

// Hàm tạo bộ nhớ SoA có Alignment cho SIMD
CellSoA* cell_soa_create(int capacity);

// Hàm giải phóng bộ nhớ
void cell_soa_free(CellSoA* soa);

// Hàm reset các giá trị về mặc định (Tương đương reset() bên Java)[cite: 7]
void cell_soa_reset(CellSoA* soa);

// Hàm reset 1 cell cụ thể theo index
void cell_soa_reset_at(CellSoA* soa, int index);
