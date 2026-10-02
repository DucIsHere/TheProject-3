package net.prj3.data.worldgen.preset.settings;

import com.mojang.serialization.Codec;

import net.minecraft.util.StringRepresentable;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.continent.Continent;
import net.prj3.world.worldgen.cell.continent.advanced.AdvancedContinentGenerator;
import net.prj3.world.worldgen.cell.continent.fancy.FancyContinentGenerator;
import net.prj3.world.worldgen.cell.continent.infinite.InfiniteContinentGenerator;
import net.prj3.world.worldgen.cell.continent.simple.MultiContinentGenerator;
import net.prj3.world.worldgen.cell.continent.simple.SingleContinentGenerator;
import net.prj3.world.worldgen.util.Seed;

public static class ContinentType {

    public static final Codec<Continent> CODEC =
            RecordCodecBuilder.create(instance -> instance.group(
                    ContinentType.CODEC.fieldOf("continentType").forGetter(o -> o.continentType),
                    DistanceFunction.CODEC
                            .optionalFieldOf("continentShape", DistanceFunction.EUCLIDEAN)
                            .forGetter(o -> o.continentShape),
                    Codec.INT.fieldOf("continentScale").forGetter(o -> o.continentScale),
                    Codec.FLOAT.fieldOf("continentJitter").forGetter(o -> o.continentJitter),
                    Codec.FLOAT.optionalFieldOf("continentSkipping", 0.25F).forGetter(o -> o.continentSkipping),
                    Codec.FLOAT.optionalFieldOf("continentSizeVariance", 0.25F).forGetter(o -> o.continentSizeVariance),
                    Codec.INT.optionalFieldOf("continentNoiseOctaves", 5).forGetter(o -> o.continentNoiseOctaves),
                    Codec.FLOAT.optionalFieldOf("continentNoiseGain", 0.26F).forGetter(o -> o.continentNoiseGain),
                    Codec.FLOAT.optionalFieldOf("continentNoiseLacunarity", 4.33F).forGetter(o -> o.continentNoiseLacunarity)
            ).apply(instance, Continent::new));

    public final ContinentType continentType;
    public final DistanceFunction continentShape;
    public final int continentScale;
    public final float continentJitter;
    public final float continentSkipping;
    public final float continentSizeVariance;
    public final int continentNoiseOctaves;
    public final float continentNoiseGain;
    public final float continentNoiseLacunarity;

    public ContinentType(ContinentType type, DistanceFunction shape, int scale,
                     float jitter, float skipping, float sizeVariance,
                     int octaves, float gain, float lacunarity) {

        this.continentType = type;
        this.continentShape = shape;
        this.continentScale = scale;
        this.continentJitter = jitter;
        this.continentSkipping = skipping;
        this.continentSizeVariance = sizeVariance;
        this.continentNoiseOctaves = octaves;
        this.continentNoiseGain = gain;
        this.continentNoiseLacunarity = lacunarity;
    }

    public Continent copy() {
        return new Continent(
                continentType,
                continentShape,
                continentScale,
                continentJitter,
                continentSkipping,
                continentSizeVariance,
                continentNoiseOctaves,
                continentNoiseGain,
                continentNoiseLacunarity
        );
    }
}
