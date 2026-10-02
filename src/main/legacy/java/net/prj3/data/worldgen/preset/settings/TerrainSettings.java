package main.java.com.regenerationforrged.data.worldgen.preset.settings;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;

public class TerrainSettings {
    public static final Codec<TerrainSettings> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        General.CODEC.fieldOf("general").forGetter((o) -> o.general),
        Terrain.CODEC.fieldOf("steppe").forGetter((o) -> o.steppe),
        Terrain.CODEC.fieldOf("plains").forGetter((o) -> o.plains),
        Terrain.CODEC.fieldOf("hills").forGetter((o) -> o.hills),
        Terrain.CODEC.fieldOf("dales").forGetter((o) -> o.dales),
        Terrain.CODEC.fieldOf("plateau").forGetter((o) -> o.plateau),
        Terrain.CODEC.fieldOf("badlands").forGetter((o) -> o.badlands),
        Terrain.CODEC.fieldOf("torridonian").forGetter((o) -> o.torridonian),
        Terrain.CODEC.fieldOf("mountains").forGetter((o) -> o.mountains),
        Terrain.CODEC.fieldOf("volcano").forGetter((o) -> o.volcano),
        Terrain.CODEC.fieldOf("blendLow").forGetter((o) -> o.blendLow),
        Terrain.CODEC.fieldOf("blendMid").forGetter((o) -> o.blendMid),
        Terrain.CODEC.fieldOf("blendHigh").forGetter((o) -> o.blendHigh),
        MountainRanges.CODEC.fieldOf("mountainranges").forGetter((o) -> o.mountainranges)
    ).apply(instance, TerrainSettings::new));

    public General general;
    public Terrain steppe;
    public Terrain plains;
    public Terrain hills;
    public Terrain dales;
    public Terrain plateau;
    public Terrain badlands;
    public Terrain torridonian;
    public Terrain mountains;
    public Terrain volcano;
    public Terrain blendLow;
    public Terrain blendMid;
    public Terrain blendHigh;
    public MountainRanges mountainranges;

    public TerrainSettings(General general, Terrain steppe, Terrain plains, Terrain hills, Terrain dales, Terrain plateau, Terrain badlands, Terrain torridonian, Terrain mountains, Terrain volcano,
                          Terrain blendLow, Terrain blendMid, Terrain blendHigh,
                          MountainRanges mountainranges) {
        this.general = general;
        this.steppe = steppe;
        this.plains = plains;
        this.hills = hills;
        this.dales = dales;
        this.plateau = plateau;
        this.badlands = badlands;
        this.torridonian = torridonian;
        this.mountains = mountains;
        this.volcano = volcano;
        this.blendLow = blendLow;
        this.blendMid = blendMid;
        this.blendHigh = blendHigh;
        this.mountainranges = mountainranges;
    }

    public TerrainSettings copy() {
        return new TerrainSettings(this.general.copy(), this.steppe.copy(), this.plains.copy(), this.hills.copy(), this.dales.copy(), this.plateau.copy(), this.badlands.copy(), this.torridonian.copy(), this.mountains.copy(), this.volcano.copy(),
                                  this.blendLow.copy(), this.blendMid.copy(), this.blendHigh.copy(), this.mountainranges.copy());
    }

    public static class MountainRanges {
        public static final Codec<MountainRanges> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("density").forGetter((o) -> o.density),
            Sharpness.CODEC.fieldOf("sharpness").forGetter((o) -> o.sharpness),
            Codec.FLOAT.fieldOf("rangeDensity").forGetter((o) -> o.rangeDensity),
            Codec.INT.fieldOf("rangeScale").forGetter((o) -> o.rangeScale),
            Codec.FLOAT.fieldOf("scale").forGetter((o) -> o.scale),
            Codec.INT.fieldOf("weight").forGetter((o) -> o.weight)
        ).apply(instance, MountainRanges::new));

        public int density;
        public Sharpness sharpness;
        public float rangeDensity;
        public int rangeScale;
        public float scale;
        public int weight;

        public MountainRanges(int density, Sharpness sharpness, float rangeDensity, int rangeScale, float scale, int weight) {
            this.density = density;
            this.sharpness = sharpness;
            this.rangeDensity = rangeDensity;
            this.rangeScale = rangeScale;
            this.scale = scale;
            this.weight = weight;
        }

        public MountainRanges copy() {
            return new MountainRanges(this.density, this.sharpness, this.rangeDensity, this.rangeScale, this.scale, this.weight);
        }
    }

    public static class General {
        public static final Codec<General> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("terrainSeedOffset").forGetter((o) -> o.terrainSeedOffset),
            Codec.INT.fieldOf("terrainRegionSize").forGetter((o) -> o.terrainRegionSize),
            Codec.FLOAT.fieldOf("globalVerticalScale").forGetter((o) -> o.globalVerticalScale),
            Codec.FLOAT.fieldOf("globalHorizontalScale").forGetter((o) -> o.globalHorizontalScale),
            Codec.BOOL.fieldOf("fancyMountains").forGetter((o) -> o.fancyMountains)
        ).apply(instance, General::new));

        public int terrainSeedOffset;
        public int terrainRegionSize;
        public float globalVerticalScale;
        public float globalHorizontalScale;
        public boolean fancyMountains;

        public General(int terrainSeedOffset, int terrainRegionSize, float globalVerticalScale, float globalHorizontalScale, boolean fancyMountains) {
            this.terrainSeedOffset = terrainSeedOffset;
            this.terrainRegionSize = terrainRegionSize;
            this.globalVerticalScale = globalVerticalScale;
            this.globalHorizontalScale = globalHorizontalScale;
            this.fancyMountains = fancyMountains;
        }

        public General copy() {
            return new General(this.terrainSeedOffset, this.terrainRegionSize, this.globalVerticalScale, this.globalHorizontalScale, this.fancyMountains);
        }
    }

    public static class Terrain {
        public static final Codec<Terrain> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("weight").forGetter(o -> o.weight),
            Codec.FLOAT.fieldOf("baseScale").forGetter(o -> o.baseScale),
            Codec.FLOAT.fieldOf("verticalScale").forGetter(o -> o.verticalScale),
            Codec.FLOAT.fieldOf("horizontalScale").forGetter(o -> o.horizontalScale),
            Codec.FLOAT.fieldOf("baseHeight").forGetter(o -> o.baseHeight),
            Codec.FLOAT.fieldOf("slopeScale").forGetter(o -> o.slopeScale),
            Codec.FLOAT.fieldOf("valleyDepth").forGetter(o -> o.valleyDepth),
            Codec.FLOAT.fieldOf("valleyWidth").forGetter(o -> o.valleyWidth),
            Sharpness.CODEC.fieldOf("sharpness").forGetter(o -> o.sharpness), // Áp dụng Sharpness cho từng Terrain
            Codec.FLOAT.fieldOf("mountainHeightScale").forGetter(o -> o.mountainHeightScale),
            Codec.FLOAT.fieldOf("mountainScale").forGetter(o -> o.mountainScale),
            Codec.FLOAT.fieldOf("plateauHeight").forGetter(o -> o.plateauHeight),
            Codec.FLOAT.fieldOf("coastSharpness").forGetter(o -> o.coastSharpness),
            Codec.FLOAT.fieldOf("valleyErosion").forGetter(o -> o.valleyErosion),
            Codec.FLOAT.fieldOf("valleyWeirdness").forGetter(o -> o.valleyWeirdness),
            Codec.FLOAT.fieldOf("riverErosion").forGetter(o -> o.riverErosion),
            Codec.FLOAT.fieldOf("riverWeirdness").forGetter(o -> o.riverWeirdness),
            Codec.FLOAT.fieldOf("lakeWeirdness").forGetter(o -> o.lakeWeirdness),
            Codec.FLOAT.fieldOf("beachNoiseScale").forGetter(o -> o.beachNoiseScale),
            Codec.FLOAT.fieldOf("beachHeight").forGetter(o -> o.beachHeight),
            Codec.INT.fieldOf("aquiferDepthOffset").forGetter(o -> o.aquiferDepthOffset)
        ).apply(instance, Terrain::new));

        public float weight;
        public float baseScale;
        public float verticalScale;
        public float horizontalScale;
        public float baseHeight;
        public float slopeScale;
        public float valleyDepth;
        public float valleyWidth;
        public Sharpness sharpness;
        public float mountainHeightScale;
        public float mountainScale;
        public float plateauHeight;
        public float coastSharpness;
        public float valleyErosion;
        public float valleyWeirdness;
        public float riverErosion;
        public float riverWeirdness;
        public float lakeWeirdness;
        public float beachNoiseScale;
        public float beachHeight;
        public int aquiferDepthOffset;

        public Terrain(float weight, float baseScale, float verticalScale, float horizontalScale, float baseHeight, float slopeScale, float valleyDepth, float valleyWidth, Sharpness sharpness, float mountainHeightScale, float mountainScale, float plateauHeight, float coastSharpness, float valleyErosion, float valleyWeirdness, float riverErosion, float riverWeirdness, float lakeWeirdness, float beachNoiseScale, float beachHeight, int aquiferDepthOffset) {
            this.weight = weight;
            this.baseScale = baseScale;
            this.verticalScale = verticalScale;
            this.horizontalScale = horizontalScale;
            this.baseHeight = baseHeight;
            this.slopeScale = slopeScale;
            this.valleyDepth = valleyDepth;
            this.valleyWidth = valleyWidth;
            this.sharpness = sharpness;
            this.mountainHeightScale = mountainHeightScale;
            this.mountainScale = mountainScale;
            this.plateauHeight = plateauHeight;
            this.coastSharpness = coastSharpness;
            this.valleyErosion = valleyErosion;
            this.valleyWeirdness = valleyWeirdness;
            this.riverErosion = riverErosion;
            this.riverWeirdness = riverWeirdness;
            this.lakeWeirdness = lakeWeirdness;
            this.beachNoiseScale = beachNoiseScale;
            this.beachHeight = beachHeight;
            this.aquiferDepthOffset = aquiferDepthOffset;
        }

        public Terrain copy() {
            return new Terrain(weight, baseScale, verticalScale, horizontalScale, baseHeight, slopeScale, valleyDepth, valleyWidth, sharpness, mountainHeightScale, mountainScale, plateauHeight, coastSharpness, valleyErosion, valleyWeirdness, riverErosion, riverWeirdness, lakeWeirdness, beachNoiseScale, beachHeight, aquiferDepthOffset);
        }
    }
}