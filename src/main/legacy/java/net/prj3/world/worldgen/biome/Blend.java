package net.prj3.world.worldgen.biome;

public enum Blend implements BiomeParameter {
  LOW_BLEND(-1.0F, 0.35F),
  MID_BLEND(-0.5F, 0.75F),
  HIGH_BLEND(0.75F, 1.5F);

  private final float min;
  private final float max;

  private final Blend(float min, float max) {
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
