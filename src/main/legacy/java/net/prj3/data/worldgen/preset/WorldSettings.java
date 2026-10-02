package net.prj3.data.worldgen.preset.settings;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;

import net.prj3.world.worldgen.cell.continent.MushroomIslandPopulator;
import net.prj3.world.worldgen.noise.function.DistanceFunction;

public class WorldSettings {
	public static final Codec<WorldSettings> CODEC = RecordCodecBuilder.create(instance -> instance.group(
		Continent.CODEC.fieldOf("continent").forGetter((o) -> o.continent),
		ControlPoints.CODEC.fieldOf("controlPoints").forGetter((o) -> o.controlPoints),
		Properties.CODEC.fieldOf("properties").forGetter((o) -> o.properties)
	).apply(instance, WorldSettings::new));
	
    public Continent continent;
    public ControlPoints controlPoints;
    public Properties properties;
    
    public WorldSettings(Continent continent, ControlPoints controlPoints, Properties properties) {
        this.continent = continent;
        this.controlPoints = controlPoints;
        this.properties = properties;
    }
    
    public WorldSettings copy() {
    	return new WorldSettings(this.continent.copy(), this.controlPoints.copy(), this.properties.copy());
    }
    
    public static class Continent {
    	public static final Codec<Continent> CODEC = RecordCodecBuilder.create(instance -> instance.group(
    		ContinentType.CODEC.fieldOf("continentType").forGetter((o) -> o.continentType),
    		DistanceFunction.CODEC.optionalFieldOf("continentShape", DistanceFunction.EUCLIDEAN).forGetter((o) -> o.continentShape),
    		Codec.INT.fieldOf("continentScale").forGetter((o) -> o.continentScale),
    		Codec.FLOAT.fieldOf("continentJitter").forGetter((o) -> o.continentJitter),
    		Codec.FLOAT.fieldOf("continentSkipping").forGetter((o) -> o.continentSkipping),
    		Codec.FLOAT.fieldOf("continentSizeVariance").forGetter((o) -> o.continentSizeVariance),
    		Codec.INT.fieldOf("continentNoiseOctaves").forGetter((o) -> o.continentNoiseOctaves),
    		Codec.FLOAT.fieldOf("continentNoiseGain").forGetter((o) -> o.continentNoiseGain),
    		Codec.FLOAT.fieldOf("continentNoiseLacunarity").forGetter((o) -> o.continentNoiseLacunarity),
			Codec.FLOAT.fieldOf("continentWarpStrength").forGetter((o) -> o.continentsWarpStrength),
			Codec.FLOAT.fieldOf("continentWarpScale").forGetter((o) -> o.continentWarpScale)
    	).apply(instance, Continent::new));
    	
        public ContinentType continentType;
        public DistanceFunction continentShape;
        public int continentScale;
        public float continentJitter;
        public float continentSkipping;
        public float continentSizeVariance;
        public int continentNoiseOctaves;
        public float continentNoiseGain;
        public float continentNoiseLacunarity;
		public float continentWarpStrength;
		public float contientWarpScale;
        
        public Continent(ContinentType continentType, DistanceFunction continentShape, int continentScale, float continentJitter, float continentSkipping, float continentSizeVariance, int continentNoiseOctaves, float continentNoiseGain, float continentNoiseLacunarity, float continentWarpStrength, float continentWarpScale) {
            this.continentType = continentType;
            this.continentShape = continentShape;
            this.continentScale = continentScale;
            this.continentJitter = continentJitter;
            this.continentSkipping = continentSkipping;
            this.continentSizeVariance = continentSizeVariance;
            this.continentNoiseOctaves = continentNoiseOctaves;
            this.continentNoiseGain = continentNoiseGain;
            this.continentNoiseLacunarity = continentNoiseLacunarity;
			this.continentWarpStrength = continentWarpStrength;
			this.continentWarpScale = continentWarpScale;
        }
        
        public Continent copy() {
        	return new Continent(this.continentType, this.continentShape, this.continentScale, this.continentJitter, this.continentSkipping, this.continentSizeVariance, this.continentNoiseOctaves, this.continentNoiseGain, this.continentNoiseLacunarity, this.continentWarpStrength, this.continentWarpScale);
        }
    }
    
    public static class ControlPoints {
    	public static final Codec<ControlPoints> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        	Codec.FLOAT.optionalFieldOf("mushroomFieldsInland", MushroomIslandPopulator.DEFAULT_INLAND_POINT).forGetter((o) -> o.mushroomFieldsInland),
        	Codec.FLOAT.optionalFieldOf("mushroomFieldsCoast", MushroomIslandPopulator.DEFAULT_COAST_POINT).forGetter((o) -> o.mushroomFieldsCoast),
    		Codec.FLOAT.fieldOf("deepOcean").forGetter((o) -> o.deepOcean),
    		Codec.FLOAT.fieldOf("shallowOcean").forGetter((o) -> o.shallowOcean),
    		Codec.FLOAT.fieldOf("beach").forGetter((o) -> o.beach),
    		Codec.FLOAT.fieldOf("coast").forGetter((o) -> o.coast),
    		Codec.FLOAT.fieldOf("inland").forGetter((o) -> o.inland),
			Codec.FLOAT.optionalFieldOf("coastLineBlend" 0.25F).forGetter((o) -> o.coastLineBlend)
        ).apply(instance, ControlPoints::new));

    	public float mushroomFieldsInland;
    	public float mushroomFieldsCoast;
        public float deepOcean;
        public float shallowOcean;
        public float beach;
        public float coast;
        public float inland;
		public float coastLineBlend
        
        public ControlPoints(float mushroomFieldsInland, float mushroomFieldsCoast, float deepOcean, float shallowOcean, float beach, float coast, float inland, float coastLineBlend) {
        	this.mushroomFieldsInland = mushroomFieldsInland;
        	this.mushroomFieldsCoast = mushroomFieldsCoast;
            this.deepOcean = deepOcean;
            this.shallowOcean = shallowOcean;
            this.beach = beach;
            this.coast = coast;
            this.inland = inland;
			this.coastLineBlend = coastLineBlend;
        }
        
        public ControlPoints copy() {
        	return new ControlPoints(this.mushroomFieldsInland, this.mushroomFieldsCoast, this.deepOcean, this.shallowOcean, this.beach, this.coast, this.inland, this.coatsLineBlend);
        }
    }
    
    public static class Properties {
    	public static final Codec<Properties> CODEC = RecordCodecBuilder.create(instance -> instance.group(
    		SpawnType.CODEC.fieldOf("spawnType").forGetter((o) -> o.spawnType),
    		Codec.INT.fieldOf("worldHeight").forGetter((o) -> o.worldHeight),
			Codec.INT.fieldOf("worldMinY").forGetter((o) -> o.worldMinY),
			Codec.FLOAT.fieldOf("riverScale").forGetter((o) -> o.riverScale),
			Codec.INT.fieldOf("structureSeparation").forGetter((o) -> o.structureSepararion),
    		Codec.INT.optionalFieldOf("worldDepth", -64).forGetter((o) -> o.worldDepth),
    		Codec.INT.fieldOf("seaLevel").forGetter((o) -> o.seaLevel),
    		Codec.INT.optionalFieldOf("lavaLevel", -54).forGetter((o) -> o.lavaLevel)
    	).apply(instance, Properties::new));
    	
        public SpawnType spawnType;
        public int worldHeight;
		public int worldMinY;
		public float riverScale;
		public int structureSeparation;
        public int worldDepth;
        public int seaLevel;
        public int lavaLevel;
        
        public Properties(SpawnType spawnType, int worldHeight,
						  int worldMinY, float riverScale, int structureSeparation,
                        int worldDepth, int seaLevel, int lavaLevel) {
        	this.spawnType = spawnType;
        	this.worldHeight = worldHeight;
			this.worldMinY = worldMinY;
			this.riverScale = riverScale;
			this.structureSeparation = structureSeparation;
        	this.worldDepth = worldDepth;
        	this.seaLevel = seaLevel;
        	this.lavaLevel = lavaLevel;
        }
        
        public Properties copy() {
        	return new Properties(this.spawnType, this.worldHeight, 
								  this.worldMinY, this.riverScale, this.structureSeparation, this.worldDepth, this.seaLevel, this.lavaLevel);
        }
        
        @Deprecated
        public int terrainScaler() {
        	return Math.min(this.worldHeight, 640);
        }
    }
}