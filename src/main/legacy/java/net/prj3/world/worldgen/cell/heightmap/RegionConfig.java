package com.regenerationforged.world.worldgen.cell.terrain.region;

import com.regenerationforged.world.worldgen.noise.module.Noise;

public record RegionConfig(
    long seed,
    int regionSize,
    Noise warpX,
    Noise warpZ,
    int warpStrength
) {}
