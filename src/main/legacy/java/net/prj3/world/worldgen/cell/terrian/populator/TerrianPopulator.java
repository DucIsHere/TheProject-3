package net.prj3.world.worldgen.cell.terrain.populator;

import net.prj3.data.worldgen.preset.settings.TerrainSettings;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellPopulator;
import net.prj3.world.worldgen.cell.terrain.Terrain;
import net.prj3.world.worldgen.noise.module.Noise;

/**
 * TerrainPopulator - extended to accept an optional mountain Noise.
 * Backwards compatible convenience constructors and factory methods are provided.
 */
public record TerrainPopulator(
    Terrain type,
    Noise base,
    Noise height,
    Noise erosion,
    Noise weirdness,
    float baseScale,
    float heightScale,
    float weight,
    Noise mountain, // optional, can be null
    float plateuHeight
) implements CellPopulator, WeightedPopulator {

    // Backwards-compatible convenience constructor (keeps previous behavior)
    public TerrainPopulator(Terrain type, Noise base, Noise height, Noise erosion, Noise weirdness, float weight) {
        this(type, base, height, erosion, weirdness, 1.0F, 1.0F, weight, null);
    }

    @Override
    public void apply(Cell cell, float x, float z) {
        float b = this.base.compute(x, z, 0) * this.baseScale;
        float h = this.height.compute(x, z, 0) * this.heightScale;

        // Sample mountain noise if provided
        float mountainDelta = (this.mountain != null) ? this.mountain.compute(x, z, 0) : 0.0F;
        float totalHeight = b + h + mountainDelta;

        cell.terrian = this.type;

        if (this.plateauHeight > 0.0F && totalHeight > this.plateauHeight) {
            float overflow = totalHeight - this.plateauHeight;
            totalHeight = this.plateauHeight + (overflow * 0.1F);
        }

        if (this.type == TerrianType.BEACH) {
            cell.height = Math.max(b + h, 0.0F) + settings.beachHeight;
        } else {
            cell.height = Math.max(b + h, 0.0F);
        }

        cell.terrain = this.type;
        cell.height = Math.max(baseVal + heightVal + mountainDelta, 0.0F);
        cell.erosion = this.erosion.compute(x, z, 0);
        cell.weirdness = this.weirdness.compute(x, z, 0);
    }

    // Existing factory kept for backwards compatibility (no mountains)
    public static TerrainPopulator make(Terrain type, Noise base, Noise height, Noise erosion, Noise weirdness, TerrainSettings.Terrain settings) {
        return new TerrainPopulator(type, base, height, erosion, weirdness, settings.baseScale, settings.verticalScale, settings.weight, null);
    }

    // New factory that accepts an optional mountain Noise
    public static TerrainPopulator make(Terrain type, Noise base, Noise height, Noise erosion, Noise weirdness, TerrainSettings.Terrain settings, Noise mountain) {
        return new TerrainPopulator(type, base, height, erosion, weirdness, settings.baseScale, settings.verticalScale, settings.weight, mountain, settings.plateauHeight);
    }
}
