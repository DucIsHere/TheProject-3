package net.prj3.world.worldgen.biome;

// Sử dụng chung interface với Erosion và Weirdness để dễ dàng quản lý
public enum Sharpness implements BiomeParameter {
    OFF(0.0F, 0.0F),
    LOWEST(-1.0F, -0.75F),
    LOW(-0.75F, -0.35F),
    BELOW_AVERAGE(-0.35F, -0.1F),
    AVERAGE(-0.1F, 0.1F),
    ABOVE_AVERAGE(0.1F, 0.45F),
    HIGH(0.45F, 0.8F),
    HIGHEST(0.8F, 1.0F);

    private final float min;
    private final float max;

    private Sharpness(float min, float max) {
        this.min = min;
        this.max = max;
    }

    @Override
    public float min() {
        return this.min;
    }

    @Override
    public float max() {
        return this.max;
    }
}
