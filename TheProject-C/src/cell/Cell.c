#include "cell.h"
#include <string.h>

static const Cell DEFAULT_CELL = {
    .height = 0.0f,
    .height_erosion = 0.0f,
    .sediment = 0.0f,
    .gradient = 0.0f,
    .region_moisture = 0.5f,
    .region_temperature = 0.5f,
    .continentID = 0.0f,
    .continent_edge = 0.0f,
    .terrain_region_id = 0.0f,
    .terrain_region_edge = 0.0f,
    .biome_region_id = 0.0f,
    .biome_region_edge = 1.0f,
    .macro_biome_id = 0.0f,
    .river_mask = 1.0f,
    .continent_x = 0,
    .continent_Z = 0,
    .erosion_mask = false,
    .erosion = 0.0f,
    .weirdness = 0.0f,
    .temperature = 0.0f,
    .moisture = 0.0f,
    .damage = 0.0f
};

void cell_init_default(Cell* cell) {
    if (cell == nullptr) return; // nullptr chuẩn C23
    *cell = DEFAULT_CELL;
}

void cell_copy(Cell* dest, const Cell* src) {
    if (dest == nullptr || src == nullptr) return;
    *dest = *src;
}

void cell_reset(Cell* cell) {
    cell_init_default(cell);
}