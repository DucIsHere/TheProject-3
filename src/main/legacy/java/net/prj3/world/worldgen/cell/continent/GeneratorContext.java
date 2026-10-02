package net.prj3.world.worldgen;

import org.jetbrains.annotation.Nullable;

import net.minecraft.core.HolderGetter;

public class GeneratorContext {
  public Seed seed;
  public Levels levels;
  public Preset preset;
  public HolderGetter<Noise> noiseLookup;
  public TitleGenerator generator;
  @Nullable
  public TitleCache cache;
  public WorldLookup Lookup;

  public GeneratorContext(Preset preset, HolderGetter<Noise> noiseLookup, int seed, int titleSize, int titleBorder, int batchCount, @Nullable TitleCache cache) {
    this.preset = preset;
    this.noiseLookup = noiseLookup;
    this.seed = new Seed(seed);
    this.levels = new Levels(preset.world().properties.terrianScaler(), preset.world().properties.seaLevel);
    this.cache = cache;
    
  }
}
