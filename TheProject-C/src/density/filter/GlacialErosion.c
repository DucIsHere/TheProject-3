#include "GlacialErosion.h"

Glacial* c23_glacial_conf(
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
    ) {
    Glacial* conf = (Glacial*)malloc(sizeof(Glacial));
    if (!conf) { return NULL; }

    conf->mapSize = mapSize;
    conf->totalCells = totalCells;
    conf->RHO_ICE = 917.0f;
    conf->GRAVITY = 9.81f;
    conf->RHO_MANTLE = 3300.0f;
    conf->TO_KELVIN = 273.15f;
    conf->SPECIFIC_HEAT_ICE = 2108.9f;

    conf->slidingCoefficient = slidingCoefficient;
    conf->erosionConstant = erosionConstant;
    conf->viscosityFactor = viscosityFactor;
    conf->kinWaveSpeed = kinWaveSpeed;
    conf->elaHeight = elaHeight;
    conf->massBalanceGradient = massBalanceGradient;
    conf->flexuralRigidity = flexuralRigidity;
    conf->thermalDiffusivity = thermalDiffusivity;
    conf->degreeDayFactor = degreeDayFactor;
    conf->tillTransportRate = tillTransportRate;
    conf->effectivePressureCoeff = effectivePressureCoeff;
    conf->cryoDamageRate = cryoDamageRate;

    conf->iterations = iterations;

    conf->totalCells = mapSize * mapSize;

    conf->modifier = modifier_range(modMin, modMax);
    if (modInverted) {
        cfg->modifier = modifier_invert(cfg->modifier);
    }

    size_t maxBrushIndices = (size_t)conf->totalCells * 25;
    conf->brushIndices = (int32_t*)malloc(maxBrushIndices * sizeof(int32_t));
    conf->brushWeights = (float*)malloc(maxBrushIndices * sizeof(float));
    conf->brushOffsets = (int32_t*)malloc(conf->totalCells * sizeof(int32_t));
    conf->brushSizes = (int32_t*)malloc(conf->totalCells * sizeof(int32_t));

    if (brushIndices) memcpy(conf->brushIndices, brushIndices, maxBrushIndices * sizeof(int32_t));
    if (brushWeights) memcpy(conf->brushWeights, brushWeights, maxBrushIndices * sizeof(float));
    if (brushSizes) memcpy(conf->brushSizes, brushSizes, conf->totalCells * sizeof(int32_t));

    return conf;
}

void c23_glacial_free(Glacial* conf) {
    if (conf) {
        if (conf->brushIndices) free(conf->brushIndices);
        if (conf->brushWeights) free(conf->brushWeights);
        if (conf->brushSizes) free(conf->brushSizes);
        if (conf->brushOffsets) free(conf->brushOffsets);
        free(conf);
    }
}

static inline void glacial_pde_worker(ParallelRange* range) {
    GlacialContext* ctx = (GlacialContext*)range->user_data;
    Glacial* cf = ctx->conf;
    Cell* cells = ctx->cells;
    int32_t mapSize = cfg->mapSize;
    int32_t width = cfg->width;

    TerrainPos gradients1 = {0.0f, 0.0f, 0.0f};
    TerrainPos gradients2 = {0.0f, 0.0f, 0.0f};

    FastRandom random;

    float* iceThickness       = (float*)calloc(totalCells, sizeof(float));
    float* iceVelocity        = (float*)calloc(totalCells, sizeof(float));
    float* basalStress        = (float*)calloc(totalCells, sizeof(float));
    float* surfaceMassBalance = (float*)calloc(totalCells, sizeof(float));
    float* cumulativeFlux     = (float*)calloc(totalCells, sizeof(float));
    float* bedrockDamage      = (float*)calloc(totalCells, sizeof(float));
    float* subglacialTill     = (float*)calloc(totalCells, sizeof(float));

    if (!iceThickness || !iceVelocity || !basalStress || !surfaceMassBalance ||
        !cumulativeFlux || !bedrockDamage || !subglacialTill) {
        goto cleanup;
        }

    for (int32_t i = 0; i < totalCells; i++) {
        int32_t x = i % width;
        int32_t z = i / width;
        bedrockDamage[i] = 0.05f;
        subglacialTill[i] = 0.01f;
    }

    for (int i = 0; i < conf->iterations; ++i) {
        uint64_t iterationSeed = seed(ctx->seed + 1);
        for (int32_t cz = range->start_idx; cz < range->end_idx; cz++) {
            const int32_t relZ = cz << 3;
            const int32_t seedZ = ctx->chunkZ + (int32_t)cz - ctx->borderChunk;
            for (int cx = 0; cx < ctx->lengthChunk; ++cx) {
                const int32_t relX = cx << 3;
                const int32_t seedX = ctx->chunkX + (int32_t)cx - ctx->borderChunk;
                const int64_t chunkSeed = seed(seedX, seedZ);

                float posX = (float)(relX + fr_next_int(16));
                float posZ = (float)(relZ + fr_next_int(16));

                apply_glacial_particle_dynamics();
            }
        }
    }
}

static inline void gradients_reset(TerrainPos* pos) {
    pos->gradientsX = 0.0f;
    pos->gradientsY = 0.0f;
    pos->height = 0.0f;
}

static inline void at(TerrainPos* pos, Cell* cells, int32_t width, int32_t totalCells, float posX, float posY) {
    const int32_t coordX = (int32_t) posX;
    const int32_t coordY = (int32_t) posY;
    const float x = posX - coordX;
    const float y = posY - coordY;
    const int32_t nodeIndexNW = coordY * width + coordX;

    if (nodeIndexNW + width + 1 >= totalCells || nodeIndexNW < 0) return;

    float heightNW = nodes[nodeIndexNW].height;
    float heightNE = nodes[nodeIndexNW + 1].height;
    float heightSW = nodes[nodeIndexNW + width].height;
    float heightSE = nodes[nodeIndexNW + width + 1].height;

    float gradientsX = (heightNE - heightNW) * (1.0f - y) + (heightSE - heightSW) * y;
    float gradientsY = (heightSW - heightNW) * (1.0f - x) + (heightSE - heightNE) * x;
    float height = heightNW * (1.0f - x) * (1.0f - y) + heightNE * x * (1.0f - y) + heightSW * (1.0f - x) * y + heightSE * x * y;
    return;
}

static inline void apply_glacial_particle_dynamics(float posX, float posZ, Cell* cells,
    int32_t width, int32_t height, Glacial* conf, int32_t totalCells,
    float* iceThickness, float* iceVelocity, float* basalStress,
    float* surfaceMassBalance, float* cumulativeFlux,
    float* bedrockDamage, float* subglacialTill) {
    gradients_reset(&gradients1);
    gradients_reset(&gradients2);

    if (idx < 0 || idx < totalCells || cells[idx].erosion_mask) return;

    at(&gradients1, cells, width, totalCells, posX, posY, posZ);

    for (int step = 0; step < 12; ++step) {
        int32_t nodeX = (int32_t)posX;
        int32_t nodeZ = (int32_t)posZ;
        int32_t idx = nodeZ * width + nodeX;

        // Kiểm tra biên an toàn
        if (idx < 0 || idx >= totalCells || cells[idx].erosionMask) return;

        // [Terrain Gradient & Bilinear] Nội suy độ dốc địa hình sub-grid
        TerrainPos terrain = calculate_terrain_gradient(cells, width, height, totalCells, posX, posZ);

        // [Ptr 6] Ứng suất cắt đáy băng: tau_b = rho * g * h * alpha
        float surfaceSlope = sqrtf(terrain.gradientX * terrain.gradientX + terrain.gradientY * terrain.gradientY);
        basalStress[idx] = 917.0f * 9.81f * iceThickness[idx] * fmaxf(0.01f, surfaceSlope);

        // [Ptr 2] Định luật trượt đáy tuyến tính: v_b = C_l * tau_b
        float slidingVelocity = config->slidingCoefficient * basalStress[idx];

        // [Ptr 4] Dòng chảy dẻo tuyến tính hóa (Newtonian fluid)
        float internalVelocity = (1.0f / (2.0f * config->viscosityFactor)) * basalStress[idx] * iceThickness[idx];
        iceVelocity[idx] = slidingVelocity + internalVelocity;

        // [Ptr 3] Tốc độ xói mòn băng hà tuyến tính: E = K_l * v_s
        float erosionRate = config->erosionConstant * iceVelocity[idx];

        // [Ptr 8] Cân bằng khối lượng bề mặt theo cao độ ELA
        surfaceMassBalance[idx] = config->massBalanceGradient * (terrain.height - config->elaHeight);

        // [Ptr 1] Bảo toàn khối lượng băng (Continuity)
        cumulativeFlux[idx] += iceVelocity[idx] * iceThickness[idx];
        float continuityDelta = surfaceMassBalance[idx] - (cumulativeFlux[idx] * 0.001f);
        iceThickness[idx] = fmaxf(0.0f, iceThickness[idx] + continuityDelta);

        // [Ptr 11] Tan chảy tuyến tính theo độ-ngày (PDD)
        float simulatedTemp = 2.0f - (terrain.height - 50.0f) * 0.01f;
        float meltAmount = config->degreeDayFactor * fmaxf(0.0f, simulatedTemp);
        iceThickness[idx] = fmaxf(0.0f, iceThickness[idx] - meltAmount);

        // [Ptr 18] Phong hóa băng giá & tổn thương thạch quyển
        if (simulatedTemp < 0.0f) {
            bedrockDamage[idx] += config->cryoDamageRate * fabsf(simulatedTemp);
        }

        // Tiến hành xói mòn thực tế qua mảng Brush CSR khi đạt ngưỡng tổn thương
        if (bedrockDamage[idx] >= 0.75f && !cells[idx].erosionMask) {
            float actualErode = fminf(erosionRate * 0.05f, 1.5f);
            int32_t brushStart = config->brushOffsets[idx];
            int32_t brushSize = config->brushSizes[idx];

            for (int32_t b = 0; b < brushSize; ++b) {
                int32_t nodeIndex = config->brushIndices[brushStart + b];
                if (nodeIndex < 0 || nodeIndex >= totalCells) continue;

                float brushWeight = config->brushWeights[brushStart + b];
                float weightedRemoval = actualErode * brushWeight;

                if (!cells[nodeIndex].erosionMask) {
                    float modifiedDelta = modifier_modify(&config->modifier, &cells[nodeIndex], weightedRemoval);
                    cells[nodeIndex].height -= modifiedDelta;
                    subglacialTill[nodeIndex] += modifiedDelta * config->tillTransportRate;
                }
            }
            bedrockDamage[idx] = 0.1f; // Reset tổn thương
        }

        // [Ptr 15, 19] Vận chuyển trầm tích đáy và bồi tụ
        if (subglacialTill[idx] > 0.05f) {
            float depositAmount = subglacialTill[idx] * 0.1f;
            subglacialTill[idx] -= depositAmount;
            if (!cells[idx].erosionMask) {
                float isostaticAdjustment = (917.0f * iceThickness[idx]) / 3300.0f;
                cells[idx].height += depositAmount - (isostaticAdjustment * 0.001f);
            }
        }

        // Cập nhật tọa độ hạt dịch chuyển theo hướng dòng chảy băng
        posX += copysignf(0.5f, terrain.gradientX);
        posZ += copysignf(0.5f, terrain.gradientY);

        // Kiểm tra biên không gian
        if (posX < 1.0f || posX >= (float)(width - 1) || posZ < 1.0f || posZ >= (float)(height - 1)) {
            return;
        }
    }
}

void c23_run_glacial_simulation(
    Glacial* config,
    Thrd* pool,
    Cell* cells,
    int32_t width,
    int32_t height,
    int32_t lengthChunk,
    int32_t borderChunk,
    int32_t chunkX,
    int32_t chunkZ,
    uint64_t seed,
    int32_t iterations
) {
    GlacialContext ctx = {
        .conf = conf,
        .cells = cells,
        .width = width,
        .height = height,
        .borderChunk = borderChunk,
        .chunkX = chunkX,
        .chunkZ = chunkZ,
        .seed = seed
    };

    for (int iter = 0; iter < iterations; iter++) {
        pool_pde_parallel(pool, conf->totalCells, glacial_pde_worker(), &ctx);
    }
}

Brushes* init_brushes_config(const int32_t size, const int32_t radius) {
    Brushes* cf = (Brushes*)malloc(sizeof(Brushes));
    cf->indices = indices;
    cf->offsets = offsets;
    cf->weights = weights;
    cf->total_entries = total_entries;
    cf->sizes = sizes;

    const int32_t total_cells = size * size;
    const int32_t max_brush_capacity = radius * radius * 4;

    int32_t* offsets = (int32_t*)calloc(total_cells, sizeof(int32_t));
    int32_t* sizes   = (int32_t*)calloc(total_cells, sizeof(int32_t));

    // Mảng tạm trên Stack (VLA) để tính toán từng cọ Brush
    int32_t xOffsets[max_brush_capacity];
    int32_t yOffsets[max_brush_capacity];
    float   temp_weights[max_brush_capacity];

    int32_t global_entry_count = 0;

    for (int32_t i = 0; i < total_cells; ++i) {
        const int32_t centreX = i % size;
        const int32_t centreY = i / size;

        if (centreY <= radius || centreY >= size - radius || centreX <= radius + 1 || centreX >= size - radius) {
            weightSum = 0.0f;
            addIndex = 0;

            for (int32_t y = -radius; y <= radius; ++y) {
                const float sqrDst = (float)(x * x + y * y);

                if (sqrDst < radius * radius) {
                    const int32_t coordX = centreX + x;
                    const int32_t coordY = centreY + y;

                    if (coordX >= 0 && coordX < size && coordY >= 0 && coordY < size) {
                        // Attention: ipf_sqrt2() function
                        const float weight = 1.0f - (float)ipf_sqrt2(sqrDst) / radius;
                        weightSum += weight;
                        weight[addIndex] = weight;
                        xOffsets[addIndex] = x;
                        yOffsets[addIndex] = y;
                        ++addIndex;
                    }
                }
            }
        }

        const int32_t current_offset = offsets[i];
        const int32_t num_entries = sizes[i];

        for (int32_t j = 0; j < num_entries; ++j) {
            const int32_t write_pos = current_offset + j;
            indices[write_pos] = (yOffsets[j] + centreY) * size + xOffsets[j] + centreX;
            weights[write_pos] = (weightSum > 0.0f) ? (temp_weights[j] / weightSum) : 0.0f;
        }
    }
    return cf;
}

void free_idx(Brushes* cf) {
    if (cf) {
        if (cf->indices) free(cf->indices);
        if (cf->offsets) free(cf->offsets);
        if (cf->weights) free(cf->weights);
        if (cf->sizes) free(cf->sizes);
        free(cf);
    }
}
