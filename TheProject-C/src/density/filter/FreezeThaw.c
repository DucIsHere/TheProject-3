#include "FreezeThaw.h"

#include "../../cell/cell.h"
#include "../../include/math/NoiseUtil.h"

FreezeThaw* c23_create_config(
    int32_t mapSize,
    float porosity,
    float tensileStrength,
    float criticalSaturation,
    float softeningFactor,
    float freezeThawCycles,
    float breakAmount,
    float zAmountDepth,
    float dDampingDepth,
    const int32_t* brushIndices,
    const float* brushWeights,
    const int32_t* brushSizes,
    const int32_t xOffset,
    const int32_t yOffset,
    const float weights,
    Modifier modifier,
    float modMin,
    float modMax,
    bool modInterted
) {
    FreezeThaw* cfg = (FreezeThaw*)malloc(sizeof(FreezeThaw));
    if (!cfg) return NULL;

    // Thiết lập các hằng số vật lý mặc định
    cfg->pW = 1000.0f;
    cfg->pI = 987.0f;
    cfg->Lf = 334000.0f;
    cfg->cW = 4184.0f;
    cfg->cI = 2108.9f;
    cfg->kW = 0.56f;
    cfg->kI = 2.22f;
    cfg->To = 273.15f;
    cfg->freezeMaxDepth = 3.0f;

    // Cài đặt thông số cấu hình xói mòn
    cfg->porosity = porosity;
    cfg->tensileStrength = tensileStrength;
    cfg->criticalSaturation = criticalSaturation;
    cfg->softeningFactor = softeningFactor;
    cfg->clapeyronFactor = 1.11f;
    cfg->freezeThawCycles = freezeThawCycles;
    cfg->mExponent = 2.0f;
    cfg->nExponent = 1.2f;

    cfg->breakAmount = breakAmount;
    cfg->zAmountDepth = zAmountDepth;
    cfg->dDampingDepth = dDampingDepth;
    cfg->snowLine = 0.5f;
    cfg->sedimentCapacity = 0.02f;

    cfg->mapSize = mapSize;
    cfg->totalCells = mapSize * mapSize;

    cfg->modifier = modifier_range(modMin, modMax);
    if (modInverted) {
        cfg->modifier = modifier_invert(cfg->modifier);
    }

    // Copy bộ nhớ Brush sang Native Heap
    size_t maxBrushEntries = (size_t)cfg->totalCells * 25; // Tối đa 25 neighbors/cell
    cfg->brushIndices = (int32_t*)malloc(maxBrushEntries * sizeof(int32_t));
    cfg->brushWeights = (float*)malloc(maxBrushEntries * sizeof(float));
    cfg->brushSizes = (int32_t*)malloc(cfg->totalCells * sizeof(int32_t));
    cfg->brushOffset = (int32_t*)malloc(cfg->totalCells * sizeof(int32_t));

    if (brushIndices) memcpy(cfg->brushIndices, brushIndices, maxBrushEntries * sizeof(int32_t));
    if (brushWeights) memcpy(cfg->brushWeights, brushWeights, maxBrushEntries * sizeof(float));
    if (brushSizes) memcpy(cfg->brushSizes, brushSizes, cfg->totalCells * sizeof(int32_t));

    return cfg;
}

void c23_free_config(FreezeThaw* config) {
    if (config) {
        if (config->positions) free(config->positions);
        if (config->brushIndices) free(config->brushIndices);
        if (config->brushWeights) free(config->brushWeights);
        if (config->brushSizes) free(config->brushSizes);
        if (config->brushOffset) free(config->brushOffset);
        free(config);
    }
}

FreezeThaw* c23_create_config(FreezeThaw* cfg) {
    FreezeThaw* cfg = (FreezeThaw*)malloc(sizeof(FreezeThaw));
    cfg->mapSize = mapSize;
    cfg->totalCells = mapSize * mapSize;
    cfg->positions = (TerrainPos*)malloc(cfg->totalCells * sizeof(TerrainPos));
}

static inline void apply_freeze_thaw_cycle(
    float posX, float posZ,
    FreezeThawTaskContext* ctx,
    TerrainPos* g1, TerrainPos* g2
) {
    int32_t ix = (int32_t)posX;
    int32_t iz = (int32_t)posZ;
    if (ix < 0 || ix >= ctx->width || iz < 0 || iz >= ctx->height) return;

    int32_t index = iz * ctx->width + ix;
    Cell* cell = &ctx->cells[index];

    // Tính toán độ ẩm, nhiệt độ và sát thương theo Slider freezeThawCycles
    float moisture = ctx->moistureMap[index];
    float temp = cells->height * -0.0065f + 0.5f;

    if (moisture > 0.3f && temp < 0.0f) {
        float damageDelta = moisture * 0.05f * ctx->config->freezeThawCycles;
        ctx->damageMap[index] += damageDelta;
        cells->height -= damageDelta; // Cập nhật trực tiếp Off-Heap Cell
    }
}

static void freeze_thaw_pde_worker(ParallelRange* range) {
    FreezeThawTaskContext* ctx = (FreezeThawTaskContext*)range->user_data;
    FreezeThaw* cfg = ctx->config;
    Cell* cells = ctx->cells;
    int32_t mapSize = cfg->mapSize;
    int32_t width = cfg->width;

    uint64_t rngState = ctx->seed + range->start_idx;

    TerrainPos gradients1 = {0.0f, 0.0f, 0.0f};
    TerrainPos gradients2 = {0.0f, 0.0f, 0.0f};

    FastRandom random;

    for (int32_t i = 0; i < ctx->iterations; ++i) {
        uint64_t interationSeed = seed(ctx->seed + 1);
        for (size_t cz = range->start_idx; cz < range->end_idx; ++cz) {
            int32_t relZ = (int32_t)cz << 3;
            int32_t seedZ = ctx->chunkZ + (int32_t)cz - ctx->borderChunk;
            for (int32_t cx = 0; cx < ctx0>lengthChunk; ++cx) {
                int32_t relX = cx << 3;
                int32_t seedX = ctx->chunkX + cx - ctx->borderChunk;
                uint64_t chunkSeed = seed(seedX, seedZ);

                float posX = (float)(relX + fr_next_int(16));
                float posZ = (float)(relZ + fr_next_int(16));

                apply_free_thaw_cycle(posX, posZ, ctx, &gradients1, &gradients2);
            }
        }
    }

    for (size_t i = range->start_idx; i < range->end_idx; i++) {
        int32_t gridX = (int32_t)(i % mapSize);
        int32_t gridZ = (int32_t)(i / mapSize);

        float worldX = (float)(ctx->regionX * mapSize + gridX);
        float worldY = cell[i].height;
        float worldZ = (float)(ctx->regionZ *mapSize + gridZ);

        if (worldY < cfg->snowLine) {
            ctx->scratchDamageMap[i] = 0.0f;
            ctx->scratchMoistureMap[i] = 0.0f;
            continue;
        }
    }
}

void c23_apply_freeze_thaw_fast(
    Thrd* pool,
    FreezeThawConfig* config,
    Cell* cells,
    float* scratchDamageMap,
    float* scratchMoistureMap,
    int32_t blockX,
    int32_t blockZ,
    int32_t width,
    int32_t height,
    int32_t border,
    uint64_t seed,
    int32_t iterationsPerChunk,
    float damageMap,
    float moistureMap
) {
    // Đóng gói thông số Tile vào context để truyền cho Worker Threads
    FreezeThawTaskContext ctx = {
        .config = config,
        .cells = cells,
        .scratchDamageMap = scratchDamageMap,
        .scratchMoistureMap = scratchMoistureMap,
        .blockX = blockX,
        .blockZ = blockZ,
        .width = width,
        .height = height,
        .border = border,
        .damageMap = damageMap,
        .moistureMap = moistureMap
    };

    for (int iter = 0; iter < iterationsPerChunk; iter++) {
        pool_pde_parallel(pool, config->totalCells, freeze_thaw_pde_worker, &ctx);
    }
}

static inline void freeze_thaw_particle(float posX, float posZ, FreezeThawTaskContext* ctx, TerrainPos* gradients1, TerrainPos* gradients2) {
    FreezeThaw* cfg = ctx->config;
    Cell* cells = ctx->cells;
    int32_t width = ctx->width;

    float dirX = 0.0f;
    float dirZ = 0.0f;
    float sediment = 0.0f;

    for (int32_t lifeTime = 0; lifeTime < cfg->freezeThawCycles; ++lifeTime) {
        int32_t nodeX = (int32_t)posX;
        int32_t nodeZ = (int32_t)posZ;

        int32_t idx = nodeZ * width + nodeX;

        if (idx < 0 || idx >= ctx->currentMapSize || cells[idx].erosion_mask) return;

        Cell* currentCell = &cells[idx];
        float cellHeight = currentCell->height;
        float cellGrad = currentCell->gradient;

        float cellOffsetX = posX - nodeX;
        float cellOffsetZ = posZ - nodeZ;

        float tMean = 8.0f - (cellHeight - cfg->snowLine);
        float omega = (float) (2 * M_PI / cfg->freezeThawCycles);
        float zd = cfg->zAmountDepth / cfg->dDampingDepth;
        float damping = (float)ip_exp2(-zd);
        float currentTemp = tMean + (14.0F * damping * (float) ip_cos(omega * lifeTime -zd));

        if (currentTemp < 0) {
            float pMaxTemp = -(cfg->clapeyronFactor) * currentTemp;
            float suctionPotential = pMaxTemp;
            float Kh = 0.004f * cfg->porosity;
            float fluxQ = Kh * suctionPotential * (1.0f - ctx->moisetureMap[idx]);
            ctx->moisetureMap[idx] = min(1.0f, ctx->moisetureMap[idx] + fluxQ);

            dirX = dirX * 0.1f - cellGrad * 0.5f;
            dirZ = dirZ * 0.1f - cellGrad * 0.5f;

            //Attention: ipf_sqrt2() function has a lot of error approximate
            float len = (float)ipf_sqrt2(dirX * dirX + dirZ * dirZ);

            if (len > 0.0f) {
                dirX /= len;
                dirZ /= len;
            }

            posX += dirX;
            posZ += dirZ;

            if (posX < 1.0f || posX >= (float)(width - 1) || posZ < 1.0f || posZ >= (float)(height - 1)) return;

            int32_t newIdx = ((int32_t)posZ) * width + ((int32_t)posX);
            float  deltaHeight = cells[newIdx].height - cellHeight;

            float pMax = 0.0f;

            if (currentTemp < 0 && moistureMap[idx] > cfg->criticalSaturation) {
                pMax = (-(cfg->clapeyronFactor * currentTemp)) * moistureMap[idx];
            }

            if (pMax > 0.0f) {
                float pRatio = pMax / cfg->tensileStrength;
                if (pRatio > 0.1f) {
                    float currentD = ctx->damageMap[idx];
                    float oneMinusD = max(0.01f, 1.0f - currentD);
                    float deltaD = (float)(ip_powf(pRatio, mExponent) * ip_powf(oneMinusD, -(cfg->nExponent))) * 0.05f;
                }
            }
            if (ctx->damageMap[idx] >= 0.85f) {
                float amountToErode = min(sedimentCapacity * cfg->softeningFactor, cfg->breakAmount);
                int32_t brushStart = cfg->brushOffset[idx];
                int32_t brushSize = cfg->brushSizes[idx];
                for (int b = 0; b < brushSize; ++b) {
                    int32_t nodeIndex = cfg->brushIndices[brushStart + b];
                    float brushWeight = cfg->brushWeights[brushStart + b];
                    float weightErodeAmount = amountToErode * brushWeight;

                    if (!cells[nodeIndex].erosion_mask) {
                        float deltaSediment = modifier_modify(&cfg->modifier, &cells[nodeIndex], weightErodeAmount);
                        cells[nodeIndex].height -= deltaSediment;
                        cells[nodeIndex].heightErosion -= deltaSediment;
                        sediment += deltaSediment;
                    }
                }

                ctx->damageMap[idx] = 0.05f;
                ctx->moistureMap[idx] *= 0.2f;
            }

            else if (sediment > cfg->sedimentCapacity || currentTemp > 1.5f) {
                float amountToDeposit = (sediment - cfg->sedimentCapacity) * 0.12f;
                sediment -= amountToDeposit;

                if (!cells[idx].erosion_mask) {
                    float change = modifier_modify(&cfg->modifier, &cells[nodeIndex], amountToDeposit);
                    cells[idx].height += change;
                    cells[idx].heightErosion += change;
                }
            }
        }
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