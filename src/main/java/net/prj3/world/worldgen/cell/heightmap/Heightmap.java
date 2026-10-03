package net.prj3.world.worldgen.cell.heightmap;

import net.minecraft.core.HolderGetter;

import net.prj3.data.worldgen.preset.PresetNoiseData;
import net.prj3.data.worldgen.preset.PresetTerrainTypeNoise;
import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.data.worldgen.preset.settings.TerrainSettings;
import net.prj3.data.worldgen.preset.settings.WorldSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.biome.Erosion;
import net.prj3.world.worldgen.biome.Weirdness;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellPopulator;
import net.prj3.world.worldgen.cell.climate.Climate;
import net.prj3.world.worldgen.cell.continent.Continent;
import net.prj3.world.worldgen.cell.continent.ContinentLerper2;
import net.prj3.world.worldgen.cell.continent.ContinentLerper3;
import net.prj3.world.worldgen.cell.rivermap.Rivermap;
import net.prj3.world.worldgen.cell.terrain.Blender;
import net.prj3.world.worldgen.cell.terrain.Populators;
import net.prj3.world.worldgen.cell.terrain.TerrainType;
import net.prj3.world.worldgen.cell.terrain.populator.VolcanoPopulator;
import net.prj3.world.worldgen.cell.terrain.provider.TerrainProvider;
import net.prj3.world.worldgen.cell.terrain.region.RegionLerper;
import net.prj3.world.worldgen.cell.terrain.region.RegionModule;
import net.prj3.world.worldgen.cell.terrain.region.RegionSelector;
import net.prj3.world.worldgen.noise.function.DistanceFunction;
import net.prj3.world.worldgen.noise.function.EdgeFunction;
import net.prj3.world.worldgen.noise.function.Interpolation;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.noise.module.Noises;
import net.prj3.world.worldgen.util.Seed;

public record Heightmap(CellPopulator terrain, CellPopulator region, Continent continent, Climate climate, Levels levels, ControlPoints controlPoints, float terrainFrequency, Noise beachNoise) {

    public void apply(Cell cell, float x, float z, boolean applyClimate) {
        this.applyTerrain(cell, x, z);
        this.applyRivers(cell, x, z, this.continent.getRivermap(cell));
        this.applyClimate(cell, x, z, applyClimate);
    }

    public void applyTerrain(Cell cell, float x, float z) {
        cell.terrain = TerrainType.FLATS;
        cell.beachNoise = this.beachNoise.compute(x, z, 0);

        this.continent.apply(cell, x, z);
        this.region.apply(cell, x, z);
        this.terrain.apply(cell, x * this.terrainFrequency, z * this.terrainFrequency);
    }

    public void applyRivers(Cell cell, float x, float z, Rivermap rivermap) {
        rivermap.apply(cell, x, z);
        VolcanoPopulator.modifyVolcanoType(cell, this.levels);
    }

    public void applyClimate(Cell cell, float x, float z, boolean applyClimate) {
        float riverValleyThreshold = 0.675F;
        if(cell.riverMask < riverValleyThreshold) {
            cell.erosion = 0.445F;
            cell.weirdness = 0.34F;
        }

        if(cell.terrain.isRiver()) {
            cell.erosion = -0.05F;
            cell.weirdness = -0.03F;
        }

        if(cell.terrain.isLake() && cell.height < this.levels.water) {
            cell.erosion = Erosion.LEVEL_4.mid();
            cell.weirdness = -0.03F;
        }
        if(cell.terrain.isWetland()) {
            cell.erosion = Erosion.LEVEL_6.mid();
            cell.weirdness = Weirdness.VALLEY.mid();
        }

        this.climate.apply(cell, x, z, applyClimate);

        if(cell.riverMask >= riverValleyThreshold && cell.macroBiomeId > 0.5F) {
            cell.weirdness = -cell.weirdness;
        }
    }

    public static Heightmap make(GeneratorContext context) {
        HolderGetter<Noise> noiseLookup = context.noiseLookup;

        Preset preset = context.preset;
        WorldSettings world = context.preset.world();
        ControlPoints controlPoints = ControlPoints.make(world.controlPoints);

        TerrainSettings terrainSettings = preset.terrain();
        TerrainSettings.General general = terrainSettings.general;
        float globalVerticalScale = general.globalVerticalScale;

        Seed regionWarp = context.seed.offset(8934);
        int regionWarpScale = 400;
        int regionWarpStrength = 200;

        RegionConfig regionConfig = new RegionConfig(
                context.seed.root() + 789124,
                general.terrainRegionSize,
                Noises.simplex(regionWarp.next(), regionWarpScale, 1),
                Noises.simplex(regionWarp.next(), regionWarpScale, 1),
                regionWarpStrength
        );
        Levels levels = context.levels;
        float terrainFrequency = 1.0F / terrainSettings.general.globalHorizontalScale;
        CellPopulator region = new RegionModule(regionConfig);

        Seed mountainSeed = context.seed.offset(general.terrainSeedOffset);
        Noise mountainShape = Noises.worleyEdge(mountainSeed.next(), 1000, EdgeFunction.DISTANCE_2_ADD, DistanceFunction.EUCLIDEAN);
        mountainShape = Noises.warpPerlin(mountainShape, mountainSeed.next(), 333, 2, 250.0F);
        mountainShape = Noises.curve(mountainShape, Interpolation.CURVE3);
        mountainShape = Noises.clamp(mountainShape, 0.0F, 0.9F);
        mountainShape = Noises.map(mountainShape, 0.0F, 1.0F);

        Noise ground = PresetNoiseData.getNoise(noiseLookup, PresetTerrainTypeNoise.GROUND);

        CellPopulator terrainRegions = new RegionSelector(TerrainProvider.generateTerrain(context.seed, terrainSettings, regionConfig, levels, noiseLookup));
        CellPopulator terrainRegionBorders = Populators.makeBorder(context.seed, ground, terrainSettings.plains, terrainSettings.steppe, globalVerticalScale);
        CellPopulator terrainBlend = new RegionLerper(terrainRegionBorders, terrainRegions);
        CellPopulator mountains = Populators.makeMountainChain(mountainSeed, ground, terrainSettings.mountains, globalVerticalScale, general.fancyMountains);
        Continent continent = world.continent.continentType.create(context.seed, context);
        Climate climate = Climate.make(continent, context);
        CellPopulator land = new Blender(mountainShape, terrainBlend, mountains, 0.3F, 0.8F, 0.575F);

        CellPopulator deepOcean = Populators.makeDeepOcean(context.seed.next(), levels.water);
        CellPopulator shallowOcean = Populators.makeShallowOcean(context.levels);
        CellPopulator coast = Populators.makeCoast(context.levels);

        CellPopulator oceans = new ContinentLerper3(deepOcean, shallowOcean, coast, controlPoints.deepOcean(), controlPoints.shallowOcean(), controlPoints.coast());
        CellPopulator terrain = new ContinentLerper2(oceans, land, controlPoints.shallowOcean(), controlPoints.inland());

        Noise beachNoise = Noises.perlin2(context.seed.next(), 20, 1);
        beachNoise = Noises.mul(beachNoise, context.levels.scale(5));
        return new Heightmap(terrain, region, continent, climate, levels, controlPoints, terrainFrequency, beachNoise);
    }

    private static CellPopulator makeIslandPopulator(GeneratorContext context) {
//		float islandCoastPoint = 0.01F;
//        float islandInlandPoint = 0.005F;
//
//        CellPopulator land = (cell, x, z) -> {
//        	cell.height = context.levels.water(3);
//        };
//        CellPopulator oceans = new ContinentLerper3(deepOcean, shallowOcean, coast, controlPoints.deepOcean(), controlPoints.shallowOcean(), controlPoints.coast());
//        CellPopulator terrain = new ContinentLerper2(oceans, land, controlPoints.shallowOcean(), controlPoints.inland());
//        return terrain
        return null;
    }
}