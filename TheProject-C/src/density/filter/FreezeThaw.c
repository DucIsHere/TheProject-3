#include "FreezeThaw.h"

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
        if (config->brushIndices) free(config->brushIndices);
        if (config->brushWeights) free(config->brushWeights);
        if (config->brushSizes) free(config->brushSizes);
        free(config);
    }
}

