#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

struct Cell;
typedef static struct Cell Cell;

struct alignas(8) Cell {
    int capacity;
    float height;
    float height_erosion;
    float sediment;
    float gradient;
    float region_moisture;
    float region_temperature;
    float continentID;
    float continent_edge;
    float terrain_region_edge;
    float terrain_region_id;
    float biome_region_id;
    float biome_region_edge;
    float macro_biome_id;
    float river_mask;
    int32_t continent_x;
    int32_t continent_Z;
    float erosion_mask;
    float erosion;
    float weirdness;
    float moisture;
    float damage;
    float temperature;
} Cell;

void cell_init_default(Cell* cell);
void cell_copy(Cell* dest, const Cell* src);
void cell_reset(Cell* cell);
