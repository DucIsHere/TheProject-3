package net.prj3.data.worldgen.preset.settings;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;

public class FilterSettings {
    public static final Codec<FilterSettings> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        HydraulicErosion.CODEC.fieldOf("hydraulic").forGetter(o -> o.erosion),
        GlacialErosion.CODEC.fieldOf("glacial").forGetter(o -> o.glacial),
        AdvancedSoilFluctionSoilFluction.CODEC.fieldOf("soilFluction").forGetter(o -> o.soilFluction),
        AquiferFilter.CODEC.fieldOf("aquifer").forGetter(o -> o.aquifer),
        AdvancedSubsurfaceFlow.CODEC.fieldOf("subsurfaceFlow").forGetter(o -> o.subsurfaceFlow),
        ForceErosion.CODEC.fieldOf("force").forGetter(o -> o.force),
        LandSlide.CODEC.fieldOf("landslide").forGetter(o -> o.landSlide),
        ThermalErosion.CODEC.fieldOf("thermal").forGetter(o -> o.thermal),
        CoastalErosion.CODEC.fieldOf("coastal").forGetter(o -> o.coastal),
        PhysicalSnowAvalanche.CODEC.fieldOf("snowAvalanche").forGetter(o -> o.snowAvalanche),
        AeroErosion.CODEC.fieldOf("aeolic").forGetter(o -> o.aeolic),
        Smoothing.CODEC.fieldOf("smoothing").forGetter(o -> o.smoothing)
    ).apply(instance, FilterSettings::new));

    public HydraulicErosion hydraulic;
    public GlacialErosion glacial;
    public SoilFluction soilFluction;
    public AdvancedSubsurfaceFlow advancedSubsurfaceFlow;
    public AquiferFilter aquifer;
    public ForceErosion force;
    public LandSlide landSlide;
    public ThermalErosion thermal;
    public CoastalErosion coastal;
    public PhysicalSnowAvalanche snowAvalanche;
    public AeroErosion aeroErosion;
    public Smoothing smoothing;

    public FilterSettings(HydraulicErosion hydraulic, GlacialErosion glacial, SoilFluction soilFluction, AdvancedSubsurfaceFlow advancedSubsurfaceFlow, Aquifer aquifer,
                          ForceErosion force, LandSlide landSlide, ThermalErosion thermal, CoastalErosion coastal,
                          PhysicalSnowAvalanche snowAvalanche, AeroErosion aeolic, Smoothing smoothing) {
        this.hydraulic = hydraulic;
        this.glacial = glacial;
        this.soilFluction = soilFluction;
        this.advancedSubsurfaceFlow = advancedSubsurfaceFlow;
        this.aquifer = aquifer;
        this.force = force;
        this.landSlide = landSlide;
        this.thermal = thermal;
        this.coastal = coastal;
        this.snowAvalanche = snowAvalanche;
        this.aeolic = aeolic;
        this.smoothing = smoothing;
    }

    public FilterSettings copy() {
        return new FilterSettings(
            this.hydraulic.copy(),
            this.glacial.copy(),
            this.soilFluction.copy(),
            this.advancedSubsurfaceFlow.copy(),
            this.aquifer.copy(),
            this.force.copy(),
            this.thermal.copy(),
            this.coastal.copy(),
            this.snowAvalanche.copy(),
            this.landSlide.copy(),
            this.aeolic.copy(),
            this.smoothing.copy()
        );
    }

    // ================= EROSION =================
    public static class HydraulicErosion {
        public static final Codec<HydraulicErosion> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("dropletsPerChunk").forGetter(o -> o.dropletsPerChunk),
            Codec.INT.fieldOf("dropletLifetime").forGetter(o -> o.dropletLifetime),
            Codec.FLOAT.fieldOf("dropletVolume").forGetter(o -> o.dropletVolume),
            Codec.FLOAT.fieldOf("dropletVelocity").forGetter(o -> o.dropletVelocity),
            Codec.FLOAT.fieldOf("erosionRate").forGetter(o -> o.erosionRate),
            Codec.FLOAT.fieldOf("depositeRate").forGetter(o -> o.depositeRate),
            Codec.FLOAT.fieldOf("fluvialStreamPowerK").forGetter(o -> o.fluvialStreamPowerK),
            Codec.FLOAT.fieldOf("fluvialFluxExplonent").forGetter(o -> o.fluvialFluxExponent),
            Codec.FLOAT.fieldOf("fluvialSlopeExponent").forGetter(o -> o.fluvialSlopeExponent),
            Codec.FLOAT.fieldOf("minFluvialErosion").forGetter(o -> o.minFluvialErosion),
            Codec.FLOAT.fieldOf("maxFluvialErosion").forGetter(o -> o.maxFluvialErosion),
            Codec.FLOAT.fieldOf("lateralErosionFactor").forGetter(o -> o.lateralErosionFactor)
        ).apply(instance, HydraulicErosion::new));

        public int dropletsPerChunk;
        public int dropletLifetime;
        public float dropletVolume;
        public float dropletVelocity;
        public float erosionRate;
        public float depositeRate;
        public float fluvialStreamPowerK;
        public float fluvialFluxExponent;
        public float fluvialSlopeExponent;
        public float minFluvialErosion;
        public float maxFluvialErosion;
        public float lateralErosionFactor;

        public HydraulicErosion(int dropletsPerChunk, int dropletLifetime, float dropletVolume,
                                float dropletVelocity, float erosionRate, float depositeRate,
                                float fluvialStreamPowerK, float fluvialFluxExponent, float fluvialSlopeExponent,
                                float minFluvialErosion, float maxFluvialErosion, float lateralErosionFactor) {
            this.dropletsPerChunk = dropletsPerChunk;
            this.dropletLifetime = dropletLifetime;
            this.dropletVolume = dropletVolume;
            this.dropletVelocity = dropletVelocity;
            this.erosionRate = erosionRate;
            this.depositeRate = depositeRate;
            this.fluvialStreamPowerK = fluvialStreamPowerK;
            this.fluvialFluxExponent = fluvialFluxExponent;
            this.fluvialSlopeExponent = fluvialSlopeExponent;
            this.minFluvialErosion = minFluvialErosion;
            this.maxFluvialErosion = maxFluvialErosion;
            this.lateralErosionFactor = lateralErosionFactor;
        }

        public HydraulicErosion copy() {
            return new Erosion(dropletsPerChunk, dropletLifetime, dropletVolume,
                               dropletVelocity, erosionRate, depositeRate,
                               fluvialStreamPowerK, fluvialFluxExponent, fluvialSlopeExponent,
                               minFluvialErosion, maxFluvialErosion, lateralErosionFactor);
        }
    }

    // ================= GLACIAL =================
    public static class GlacialErosion {
        public static final Codec<GlacialErosion> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("iterations").forGetter(o -> o.iterations),
            Codec.FLOAT.fieldOf("abrasionRate").forGetter(o -> o.abrasionRate),
            Codec.FLOAT.fieldOf("depositSpeed").forGetter(o -> o.depositSpeed),
            Codec.FLOAT.fieldOf("accumulationRate").forGetter(o -> o.accumulationRate),
            Codec.FLOAT.fieldOf("meltRate").forGetter(o -> o.meltRate),
            Codec.FLOAT.fieldOf("viscosity").forGetter(o -> o.viscosity),
            Codec.FLOAT.fieldOf("pluckingProbability").forGetter(o -> o.pluckingProbability),
            Codec.FLOAT.fieldOf("pluckingRate").forGetter(o -> o.pluckingRate),
            Codec.FLOAT.fieldOf("initialSpeed").forGetter(o -> o.initialSpeed),
            Codec.FLOAT.fieldOf("initialIceVolume").forGetter(o -> o.initialIceVolume),
            Codec.INT.fieldOf("maxLifeTime").forGetter(o -> o.maxLifeTime)
        ).apply(instance, GlacialErosion::new));

        public int iterations;
        public float abrasionRate;
        public float depositSpeed;
        public float accumulationRate;
        public float meltRate;
        public float viscosity;
        public float pluckingProbability;
        public float pluckingRate;
        public float initialSpeed;
        public float initialIceVolume;
        public int maxLifeTime;

        public GlacialErosion(int iterations, float abrasionRate, float depositSpeed,
                              float accumulationRate, float meltRate, float viscosity,
                              float pluckingProbability, float pluckingRate,
                              float initialSpeed, float initialIceVolume, int maxLifeTime) {

            this.iterations = iterations;
            this.abrasionRate = abrasionRate;
            this.depositSpeed = depositSpeed;
            this.accumulationRate = accumulationRate;
            this.meltRate = meltRate;
            this.viscosity = viscosity;
            this.pluckingProbability = pluckingProbability;
            this.pluckingRate = pluckingRate;
            this.initialSpeed = initialSpeed;
            this.initialIceVolume = initialIceVolume;
            this.maxLifeTime = maxLifeTime;
        }

        public GlacialErosion copy() {
            return new GlacialErosion(iterations, abrasionRate, depositSpeed,
                accumulationRate, meltRate, viscosity,
                pluckingProbability, pluckingRate,
                initialSpeed, initialIceVolume, maxLifeTime);
        }
    }

    // ================= SOIL =================
    public static class SoilFluction {
        public static final Codec<SoilFluction> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("iterations").forGetter(o -> o.iterations),
            Codec.FLOAT.fieldOf("flowRate").forGetter(o -> o.flowRate),
            Codec.FLOAT.fieldOf("maxSedimentFlow").forGetter(o -> o.maxSedimentFlow),
            Codec.FLOAT.fieldOf("weatheringRate").forGetter(o -> o.weatheringRate),
            Codec.FLOAT.fieldOf("lobeFriction").forGetter(o -> o.lobeFriction)
        ).apply(instance, SoilFluction::new));

        public int iterations;
        public float flowRate;
        public float maxSedimentFlow;
        public float weatheringRate;
        public float lobeFriction;

        public SoilFluction(int iterations, float flowRate, float maxSedimentFlow,
                            float weatheringRate, float lobeFriction) {
            this.iterations = iterations;
            this.flowRate = flowRate;
            this.maxSedimentFlow = maxSedimentFlow;
            this.weatheringRate = weatheringRate;
            this.lobeFriction = lobeFriction;
        }

        public SoilFluction copy() {
            return new SoilFluction(iterations, flowRate, maxSedimentFlow, weatheringRate, lobeFriction);
        }
    }

    public static class AdvancedSubsurfaceFlow {
        public static final Codec<AdvancedSubsurfaceFlow> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("baseViscosity").forGetter(o -> o.baseViscosity),
            Codec.FLOAT.fieldOf("solubility").forGetter(o -> o.solubility),
            Codec.FLOAT.fieldOf("riverThreshold").forGetter(o -> o.riverThreshold),
            Codec.FLOAT.fieldOf("flowMomentum").forGetter(o -> o.flowMomentum),
            Codec.FLOAT.fieldOf("infiltrationRate").forGetter(o -> o.infiltrationRate),
            Codec.INT.fieldOf("iterations").forGetter(o -> o.iterations)
        ).apply(instance, AdvancedSubsurfaceFlow::new));

        public float baseVisvosity;
        public float solubility;
        public float riverThreshold;
        public float flowMomentum;
        public float infiltrationRate;
        public int iterations;

        public AdvancedSubsurfaceFlow(float baseViscosity, float solubility, float riverThreshold, float flowMomentum, float infiltrationRate, int iterations) {
            this.baseVisvosity = baseViscosity;
            this.solubility = solubility;
            this.riverThreshold = riverThreshold;
            this.flowMomentum = flowMomentum;
            this.infiltrationRate = infiltrationRate;
            this.iterations = iterations;
        }

        public AdvancedSubsurfaceFlow copy() {
            return new AdvancedSubsurfaceFlow(baseVisvosity, solubility, riverThreshold, flowMomentum, infiltrationRate, iterations);
        }
    }

    public static class AquiferFilter {
        public static final Codec<AquiferFilter> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("basePorosity").forGetter(o -> o.basePorosity),
            Codec.FLOAT.fieldOf("permeabilityScale").forGetter(o -> o.permeabilityScale),
            Codec.FLOAT.fieldOf("rechargeRate").forGetter(o -> o.rechargeRate),
            Codec.FLOAT.fieldOf("pressureFactor").forGetter(o -> o.pressureFactor),
            Codec.FLOAT.fieldOf("iterations").forGetter(o -> iterations),
            Codec.BOOL.OptionalFieldOf("simulateFlow").forGetter(o -> o.simulateFlow)
        ).apply(instance, AquiferFilter::new));

        public float basePorosity;
        public float permeabilityScale;
        public float rechargeRate;
        public float pressureFactor;
        public float iterations;
        public Boolean simulateFlow;

        public AquiferFilter(float basePorosity, float permeabilityScale, float rechargeRate, float pressureFactor, float iterations, Boolean simulateFlow) {
            this.basePorosity = basePorosity;
            this.permeabilityScale = permeabilityScale;
            this.rechargeRate = rechargeRate;
            this.pressureFactor = pressureFactor;
            this.iterations = iterations;
            this.simulateFlow = simulateFlow;
        }

        public AquiferFilter copy() {
            return new AquiferFilter(basePorosity, permeabilityScale, rechargeRate, pressureFactor, iterations, simulateFlow);
        }
    }

    public static class ForceErosion {
        public static final Codec<ForceErosion> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("faultDepth").forGetter(o -> o.faultDepth),
            Codec.FLOAT.fieldOf("faultThreshold").forGetter(o -> o.faultThreshold),
            Codec.FLOAT.fieldOf("faultPower").forGetter(o -> o.faultPower),
            Codec.FLOAT.fieldOf("upliftHeight").forGetter(o -> o.upliftHeight),
            Codec.FLOAT.fieldOf("upliftThreshold").forGetter(o -> o.upliftThreshold)
        ).apply(instance, ForceErosion::new));

        public float faultDepth;
        public float faultThreshold;
        public float faultPower;
        public float upliftHeight;
        public float upliftThreshold;

        public ForceErosion(float faultDepth, float faultPower, float faultThreshold, float upliftHeight, float upliftThreshold) {
            this.faultDepth = faultDepth;
            this.faultPower = faultPower;
            this.faultThreshold = faultThreshold;
            this.upliftHeight = upliftHeight;
            this.upliftThreshold = upliftThreshold;
        }

        public ForceErosion copy() {
            return new ForceErosion(this.faultDepth.copy(), this.faultPower.copy(), this.faultThreshold.copy(),
                                    this.upliftHeight.copy(), this.upliftThreshold.copy());
        }
    }

    public static class AeroErosion {
        public static final Codec<AeroErosion> CODEC= RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("erodeStrength").forGetter(o -> o.erodeStrength),
            Codec.FLOAT.fieldOf("depositStrength").forGetter(o -> o.depositStrength),
            Codec.FLOAT.fieldOf("minHeight").forGetter(o -> o.minHeight)
        ).apply(instance, AeroErosion::new));

        public float erodeStrength;
        public float depositStrength;
        public float minHeight;

        public AeroErosion(float erodeStrength, float depositStrength, float minHeight) {
            this.erodeStrength = erodeStrength;
            this.depositStrength = depositStrength;
            this.minHeight = minHeight;
        }

        public AeroErosion copy() {
            return new AeroErosion(this.erodeStrength.copy(), this.depositStrength.copy(), this.minHeight.copy());
        }
    }

    public static final class CoastalErosion {
        public static final Codec<CoastalErosion> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("baseWaterLevel").forGetter(o -> o.baseWaterLevel),
            Codec.FLOAT.fieldOf("erosionScale").forGetter(o -> o.erosionScale),
            Codec.FLOAT.fieldOf("abrasionFactor").forGetter(o -> o.abrasionFactor),
            Codec.FLOAT.fieldOf("tideAmplitude").forGetter(o -> o.tideAmplitude),
            Codec.FLOAT.fieldOf("tideFrequency").forGetter(o -> o.tideFrequency),
            Codec.FLOAT.fieldOf("criticalAngle").forGetter(o -> o.criticalAngle)
        ).apply(instance, CoastalErosion::new));
    

        public float baseWaterLevel;
        public float erosionScale;
        public float abrasionFactor;
        public float tideAmplitude;
        public float tideFrequency;
        public float criticalAngle;

        public CoastalErosion(float baseWaterLevel, float erosionScale, float abrasionFactor, float tideAmplitude, float tideFrequency, float criticalAngle) {
            this.baseWaterLevel = baseWaterLevel;
            this.erosionScale = erosionScale;
            this.abrasionFactor = abrasionFactor;
            this.tideAmplitude = tideAmplitude;
            this.tideFrequency = tideFrequency;
            this.criticalAngle = criticalAngle;
    }

        public CoastalErosion copy() {
            return new CoastalErosion(this.baseWaterLevel.copy(), this.erosionScale.copy(), this.abrasionFactor.copy(), this.tideAmplitude.copy(), this.tideFrequency.copy(), this.criticalAngle.copy());
        }
    }

    public static class ThermalErosion {
        public static final Codec<ThermalErosion> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("talusThreshold").forGetter(o -> o.talusThreshold),
            Codec.FLOAT.fieldOf("materialTransfer").forGetter(o -> o.materialTransfer)
        ).apply(instance, ThermalErosion::new));
    }

    public float talusThreshold;
    public float materialTransfer;

    public ThermalErosion(float talusThreshold, float materialTransfer) {
        this.talusThreshold = talusThreshold;
        this.materialTransfer = materialTransfer;
    }

    public ThermalErosion copy() {
        return new ThermalErosion(this.talusThreshold.copy(), this.materialTransfer.copy());
    }

    public static class PhysicalSnowAvalanche {
        public static final Codec<PhysicalSnowAvalanche> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("gravity").forGetter(o -> o.gravity),
            Codec.FLOAT.fieldOf("MU").forGetter(o -> o.MU),
            Codec.FLOAT.fieldOf("XI").forGetter(o -> o.XI),
            Codec.FLOAT.fieldOf("minSnowHeight").forGetter(o -> o.minSnowHeight),
            Codec.FLOAT.fieldOf("tensileStrength").forGetter(o -> o.tensileStrength),
            Codec.FLOAT.fieldOf("iterations").forGetter(o -> o.iterations)
        ).apply(instance, PhysicalSnowAvalanche::new));

        public float gravity;
        public float MU;
        public float XI;
        public float minSnowHeight;
        public float tensileStrength;
        public float iterations

        public PhysicalSnowAvalanche(float gravity, float MU, float XI,
                                     float minSnowHeight, float tensileStrength, float iterations) {
            this.gravity = gravity;
            this.MU = MU;
            this.XI = XI;
            this.minSnowHeight = minSnowHeight;
            this.tensileStrength = tensileStrength;
            this.iterations = iterations;
        }

        public PhysicalSnowAvalanche copy() {
            return new PhysicalSnowAvalanche(this.gravity.copy(), this.MU.copy(), this.XI.copy(), this.minSnowHeight.copy(), this.tensileStrength.copy(), this.iterations.copy());
        }
    }

    public static class LandSlide {
        public static final Codec<LandSlide> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.FLOAT.fieldOf("collapseThreshold").forGetter(o -> o.collapseThreshold),
            Codec.FLOAT.fieldOf("slideIntensity").forGetter(o -> o.slideItensity),
            Codec.FLOAT.fieldOf("sinkItensity").forGetter(o -> o.sinkIntensity),
            Codec.FLOAT.fieldOf("iterations").forGetter(o -> o.iterations)
        ).apply(instance, LandSlide::new));

        public float collapseThreshold;
        public float slideItensity;
        public float sinkIntensity;
        public float iterations;

        public LandSlide(float collapseThreshold, float slideItensity, float sinkIntensity, float iterations) {
            this.collapseThreshold = collapseThreshold;
            this.slideItensity = slideItensity;
            this.sinkIntensity = sinkIntensity;
            this.iterations = iterations;
        }

        public LandSlide copy() {
            return new LandSlide(this.collapseThreshold.copy(), this.slideItensity.copy(), this.sinkIntensity.copy(), this.iterations.copy());
        }
    }

    // ================= SMOOTHING ================
    public static class Smoothing {
        public static final Codec<Smoothing> CODEC = RecordCodecBuilder.create(instance -> instance.group(
            Codec.INT.fieldOf("iterations").forGetter(o -> o.iterations),
            Codec.FLOAT.fieldOf("smoothingRadius").forGetter(o -> o.smoothingRadius),
            Codec.FLOAT.fieldOf("smoothingRate").forGetter(o -> o.smoothingRate)
        ).apply(instance, Smoothing::new));

        public int iterations;
        public float smoothingRadius;
        public float smoothingRate;

        public Smoothing(int iterations, float smoothingRadius, float smoothingRate) {
            this.iterations = iterations;
            this.smoothingRadius = smoothingRadius;
            this.smoothingRate = smoothingRate;
        }

        public Smoothing copy() {
            return new Smoothing(iterations, smoothingRadius, smoothingRate);
        }
    }
}