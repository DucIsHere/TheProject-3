#pragma once

#include <stdbool.h>
#include "Cell.h" // Chứa struct Cell (height, terrainRegionEdge, riverMask, erosionModifier, ...)

// Struct chứa dữ liệu cấu hình Modifier trong C23
typedef struct {
    float min;
    float max;
    float range;     // max - min
    bool is_inverted; // true nếu dùng invert()
} Modifier;

// =========================================================================
// HÀM BỔ TRỢ TOÁN HỌC (TƯƠNG ĐƯƠNG NoiseUtil.java)
// =========================================================================

static inline float noise_util_map(float value, float start1, float stop1, float range1) {
    if (value <= start1) return 0.0f;
    if (value >= stop1) return 1.0f;
    return (value - start1) / range1;
}

static inline float noise_util_lerp(float start, float stop, float amount) {
    return start + (stop - start) * amount;
}

// =========================================================================
// CÁC HÀM KHỞI TẠO VÀ XỬ LÝ TƯƠNG ĐƯƠNG Modifier.java
// =========================================================================

/**
 * Tương đương: Modifier.range(float minValue, float maxValue)
 */
static inline Modifier modifier_range(float min_value, float max_value) {
    return (Modifier){
        .min = min_value,
        .max = max_value,
        .range = max_value - min_value,
        .is_inverted = false
    };
}

/**
 * Tương đương: default Modifier invert()
 */
static inline Modifier modifier_invert(Modifier mod) {
    mod.is_inverted = !mod.is_inverted;
    return mod;
}

/**
 * Tương đương: getValueModifier(float value)
 */
static inline float modifier_get_value(const Modifier* mod, float value) {
    float result;
    if (value > mod->max) {
        result = 1.0f;
    } else if (value < mod->min) {
        result = 0.0f;
    } else {
        result = (value - mod->min) / mod->range;
    }

    return mod->is_inverted ? (1.0f - result) : result;
}

/**
 * Tương đương: default float modify(Cell cell, float value)
 */
static inline float modifier_modify(const Modifier* mod, const Cell* cell, float value) {
    float strength_modifier = 1.0f;

    // 1. Lấy erosionModifier từ terrain trong Cell
    float erosion_modifier = cell->erosionModifier;
    if (erosion_modifier != 1.0f) {
        float alpha = noise_util_map(cell->terrain_region_edge, 0.0f, 0.15f, 0.15f);
        strength_modifier = noise_util_lerp(1.0f, erosion_modifier, alpha);
    }

    // 2. Kiểm tra riverMask
    if (cell->river_mask < 0.1f) {
        strength_modifier *= noise_util_map(cell->river_mask, 0.002f, 0.1f, 0.098f);
    }

    // 3. Tính modifier giá trị độ cao * strength_modifier * value gốc
    float value_mod = modifier_get_value(mod, cell->height);
    return value_mod * strength_modifier * value;
}
