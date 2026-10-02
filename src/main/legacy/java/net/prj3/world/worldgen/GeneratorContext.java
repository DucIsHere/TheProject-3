ppackage com.regenerationforrged.world.worldgen;

import org.jetbrains.annotations.Nullable;

import java.util.function.IntFunction;

import net.minecraft.core.HolderGetter;

import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.world.worldgen.cell.heightmap.Heightmap;
import net.prj3.world.worldgen.cell.heightmap.Levels;
import net.prj3.world.worldgen.cell.heightmap.WorldLookup;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.densityfunction.tile.TileCache;
import net.prj3.world.worldgen.densityfunction.tile.generation.TileGenerator;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.noise.module.Noises;
import net.prj3.world.worldgen.util.Seed;
import net.prj3.world.worldgen.densityfunction.tile.filter.AdvancedSoilFluction;
import net.prj3.world.worldgen.densityfunction.tile.filter.AdvancedSubsurfaceFlow;
import net.prj3.world.worldgen.densityfunction.tile.filter.AeroErosion;
import net.prj3.world.worldgen.densityfunction.tile.filter.AquiferFilter;
import net.prj3.world.worldgen.densityfunction.tile.filter.GlacialErosion;
import net.prj3.world.worldgen.densityfunction.tile.filter.HydarulicErosion;
import net.prj3.world.worldgen.densityfunction.tile.filter.ForceErosion;
import net.prj3.world.worldgen.densityfunction.tile.filter.ThermalErosion;
import net.prj3.world.worldgen.densityfunction.tile.filter.LandSlide;

public class GeneratorContext {
    public Seed seed;
    public Levels levels;
    public Preset preset;
    public HolderGetter<Noise> noiseLookup;
    public TileGenerator generator;
    @Nullable
    public TileCache cache;
    public WorldLookup lookup;
    
    public GeneratorContext(Preset preset, HolderGetter<Noise> noiseLookup, int seed, int tileSize, int tileBorder, int batchCount, @Nullable TileCache cache) {
        this.preset = preset;
        this.noiseLookup = noiseLookup;
        this.seed = new Seed(seed);
        this.levels = new Levels(preset.world().properties.terrainScaler(), preset.world().properties.seaLevel);
        this.generator = new TileGenerator(Heightmap.make(this), new WorldFilters(this), tileSize, tileBorder, batchCount);
        this.cache = cache;
        this.lookup = new WorldLookup(this);
    }

    public static GeneratorContext makeCached(Preset preset, HolderGetter<Noise> noiseLookup, int seed, int tileSize, int batchCount, boolean queue) {
    	GeneratorContext ctx = makeUncached(preset, noiseLookup, seed, tileSize, Math.min(2, Math.max(1, preset.filters().erosion.dropletLifetime / 16)), batchCount);
    	ctx.cache = new TileCache(tileSize, queue, ctx.generator);
    	ctx.lookup = new WorldLookup(ctx);
    	return ctx;
    }
    
    public static GeneratorContext makeUncached(Preset preset, HolderGetter<Noise> noiseLookup, int seed, int tileSize, int tileBorder, int batchCount) {
    	return new GeneratorContext(preset, noiseLookup, seed, tileSize, tileBorder, batchCount, null);
    }
}
