package net.prj3.world.worldgen.noise.module;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.prj3.world.worldgen.noise.Noise;

/**
 * ParametricMountainRange
 *
 * axisX, axisZ: two low-frequency noises which together define a 2D parametric axis point per sample.
 * ridge: high-frequency noise used to create ridged peaks.
 *
 * All inputs follow your project's Noise conventions (compute(x,z,seed) returning roughly [-1,1]).
 */
public record ParametricMountainRange(
    Noise axisX,
    Noise axisZ,
    Noise ridge,
    float invScale,
    float axisScale,
    float axisSpread,
    float halfWidth,
    float ridgeFreq,
    float amplitude,
    float sharpness
) implements Noise {
    public static final Codec<ParametricMountainRange> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        Noise.HOLDER_HELPER_CODEC.fieldOf("axis_x").forGetter(ParametricMountainRange::axisX),
        Noise.HOLDER_HELPER_CODEC.fieldOf("axis_z").forGetter(ParametricMountainRange::axisZ),
        Noise.HOLDER_HELPER_CODEC.fieldOf("ridge").forGetter(ParametricMountainRange::ridge),
        Codec.FLOAT.fieldOf("inv_scale").forGetter(ParametricMountainRange::invScale),
        Codec.FLOAT.fieldOf("axis_scale").forGetter(ParametricMountainRange::axisScale),
        Codec.FLOAT.fieldOf("axis_spread").forGetter(ParametricMountainRange::axisSpread),
        Codec.FLOAT.fieldOf("half_width").forGetter(ParametricMountainRange::halfWidth),
        Codec.FLOAT.fieldOf("ridge_freq").forGetter(ParametricMountainRange::ridgeFreq),
        Codec.FLOAT.fieldOf("amplitude").forGetter(ParametricMountainRange::amplitude),
        Codec.FLOAT.fieldOf("sharpness").forGetter(ParametricMountainRange::sharpness)
    ).apply(instance, ParametricMountainRange::new));

    @Override
    public float compute(float x, float z, int seed) {
        // Normalize to noise space
        float nx = x * this.invScale;
        float nz = z * this.invScale;

        // Sample parametric axis point (axisX, axisZ) -> world coords offset in [-1,1], map to axisSpread
        float axVal = this.axisX.compute(nx * this.axisScale, nz * this.axisScale, seed);
        float azVal = this.axisZ.compute(nx * this.axisScale, nz * this.axisScale, seed);
        float axisXPos = axVal * this.axisSpread;
        float axisZPos = azVal * this.axisSpread;

        // Euclidean distance from this point to the axis point
        float dx = x - axisXPos;
        float dz = z - axisZPos;
        float dist = (float) Math.sqrt(dx * dx + dz * dz);

        if (this.halfWidth <= 0.0F) return 0.0F;

        // Smooth mask falloff
        float t = Math.max(0.0F, 1.0F - (dist / this.halfWidth));
        float mask = t * t * (3.0F - 2.0F * t);
        if (mask <= 0.0F) return 0.0F;

        // Ridged noise: convert ridge output [-1,1] -> [0,1] then sharpen
        float rid = this.ridge.compute(nx * this.ridgeFreq, nz * this.ridgeFreq, seed);
        float ridged = 1.0F - Math.abs(rid);
        ridged = (float) Math.pow(ridged, this.sharpness);

        float mountain = mask * ridged * this.amplitude;
        return Math.max(0.0F, mountain);
    }

    @Override
    public float minValue() {
        return 0.0F;
    }

    @Override
    public float maxValue() {
        return this.amplitude;
    }

    @Override
    public Noise mapAll(Visitor visitor) {
        return visitor.apply(new ParametricMountainRange(
            this.axisX.mapAll(visitor), this.axisZ.mapAll(visitor), this.ridge.mapAll(visitor),
            this.invScale, this.axisScale, this.axisSpread, this.halfWidth,
            this.ridgeFreq, this.amplitude, this.sharpness
        ));
    }

    @Override
    public Codec<ParametricMountainRange> codec() { return CODEC; }
}
