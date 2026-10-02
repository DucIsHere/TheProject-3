package net.prj3.world.worldgen.feature.placement.poisson;

import com.mojang.serialization.Codec;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.noise.module.Noise;

public class BiomeVariance implements Noise {
    public static final BiomeVariance NONE = new BiomeVariance(null, 0.0F) {

    	@Override
        public float compute(float x, float y, int seed) {
            return NO_SPREAD;
        }
    };

    public static final float MIN_FADE = 0.025F;
    public static final float NO_SPREAD = 1F;
    private static final float SPREAD_VARIANCE = 1F;
    private static final float MAX_SPREAD = NO_SPREAD + SPREAD_VARIANCE;

    private final float fade;
    private final float range;
    private final Tile.Chunk chunk;

    public BiomeVariance(Tile.Chunk chunk, float fade) {
        this.chunk = chunk;
        this.fade = fade;
        this.range = fade - MIN_FADE;
    }

    @Override
	public float compute(float x, float z, int seed) {
        Cell cell = this.chunk.getCell((int) x, (int) z);
        float edge = 0.02F + cell.biomeRegionEdge;

        if (edge >= this.fade) {
            return NO_SPREAD;
        }

        if (edge <= MIN_FADE) {
            return MAX_SPREAD;
        }

        // alpha increases the further inside the biome region we are
        float alpha = (edge - MIN_FADE) / this.range;

        // invert so alpha increases the closer to the biome edge we are
        alpha = 1 - alpha;

        // higher output == more spread
        return NO_SPREAD + alpha * SPREAD_VARIANCE;
    }

    @Override
	public float minValue() {
		return 0.0F;
	}

	@Override
	public float maxValue() {
		return 1.0F;
	}

	@Override
	public Noise mapAll(Visitor visitor) {
		return visitor.apply(this);
	}

	@Override
	public Codec<BiomeVariance> codec() {
		throw new UnsupportedOperationException();
	}
}
