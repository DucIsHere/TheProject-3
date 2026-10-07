#include "FreezeThaw.h"

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
    const int32_t* brushSizes
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

    // Copy bộ nhớ Brush sang Native Heap
    size_t maxBrushEntries = (size_t)cfg->totalCells * 25; // Tối đa 25 neighbors/cell
    cfg->brushIndices = (int32_t*)malloc(maxBrushEntries * sizeof(int32_t));
    cfg->brushWeights = (float*)malloc(maxBrushEntries * sizeof(float));
    cfg->brushSizes = (int32_t*)malloc(cfg->totalCells * sizeof(int32_t));

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

                apply_free_thaw_cycle();
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
    int32_t iterationsPerChunk
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
        .border = border
    };

    for (int iter = 0; iter < iterationsPerChunk; iter++) {
        pool_pde_parallel(pool, config->totalCells, freeze_thaw_pde_worker, &ctx);
    }
}

static inline void appy_freeze_thaw_math(size_t index, FreezeThawTaskContext* ctx, uint64_t* rngState) {
    FreezeThaw* cfg = ctx->config;
    Cell* cells = &cfg->cells[index];
    int32_t width = ctx->width;

    int32_t lx = (int32_t)(index % width);
    int32_t lz = (int32_t)(index / width);

}
