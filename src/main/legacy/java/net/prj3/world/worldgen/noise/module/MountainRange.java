package net.prj3.world.worldgen.noise.module;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.prj3.world.worldgen.noise.Noise;

/**
 * MountainRange noise module
 *
 * Produces a non-negative mountain height contribution in blocks.
 * Fields:
 *  - axis: low-frequency noise driving the range axis (values expected in [-1,1])
 *  - ridge: high-frequency noise used to form ridged peaks (values expected in [-1,1])
 *  - invScale: convert world coords -> normalized noise coords (e.g. 1/512)
 *  - axisScale: frequency multiplier for axis noise
 *  - axisSpread: axis offset in world coordinates (how far axis moves)
 *  - halfWidth: half-width of the range in blocks
 *  - ridgeFreq: frequency multiplier for ridge noise
 *  - amplitude: maximum mountain height in blocks
 *  - sharpness: exponent to narrow ridge peaks (>1 makes sharper peaks)
 */
public record MountainRange(
	Noise axis,
	Noise ridge,
	float invScale,
	float axisScale,
	float axisSpread,
	float halfWidth,
	float ridgeFreq,
	float amplitude,
	float sharpness
) implements Noise {
	public static final Codec<MountainRange> CODEC = RecordCodecBuilder.create(instance -> instance.group(
		Noise.HOLDER_HELPER_CODEC.fieldOf("axis").forGetter(MountainRange::axis),
		Noise.HOLDER_HELPER_CODEC.fieldOf("ridge").forGetter(MountainRange::ridge),
		Codec.FLOAT.fieldOf("inv_scale").forGetter(MountainRange::invScale),
		Codec.FLOAT.fieldOf("axis_scale").forGetter(MountainRange::axisScale),
		Codec.FLOAT.fieldOf("axis_spread").forGetter(MountainRange::axisSpread),
		Codec.FLOAT.fieldOf("half_width").forGetter(MountainRange::halfWidth),
		Codec.FLOAT.fieldOf("ridge_freq").forGetter(MountainRange::ridgeFreq),
		Codec.FLOAT.fieldOf("amplitude").forGetter(MountainRange::amplitude),
		Codec.FLOAT.fieldOf("sharpness").forGetter(MountainRange::sharpness)
	).apply(instance, MountainRange::new));

	@Override
	public float compute(float x, float z, int seed) {
		// Normalize coordinates to the noise space
		float nx = x * this.invScale;
		float nz = z * this.invScale;

		// Axis noise drives an axis position (value in [-1,1]); map to world offset
		float axisVal = this.axis.compute(nx * this.axisScale, nz * this.axisScale, seed); // expected [-1,1]
		float axisPos = axisVal * this.axisSpread;

		// Distance perpendicular to axis.
		// NOTE: this implementation treats ranges roughly aligned to +X (axis varying along Z).
		// If you want arbitrary axis orientation, compute a parametric axis point instead.
		float dist = Math.abs(z - axisPos);

		// Half-width protective guard
		if (this.halfWidth <= 0.0F) return 0.0F;

		// Smooth mask falloff in [0,1]
		float t = Math.max(0.0F, 1.0F - (dist / this.halfWidth));
		float mask = t * t * (3.0F - 2.0F * t);
		if (mask <= 0.0F) return 0.0F;

		// Ridged noise: convert ridge output [-1,1] -> ridged [0,1]
		float rid = this.ridge.compute(nx * this.ridgeFreq, nz * this.ridgeFreq, seed);
		float ridged = 1.0F - Math.abs(rid);
		// sharpen peaks
		ridged = (float) Math.pow(ridged, this.sharpness);

		// Final mountain contribution
		float mountain = mask * ridged * this.amplitude;
		// Clamp safety
		if (mountain < 0.0F) return 0.0F;
		return mountain;
	}

	@Override
	public float minValue() {
		return 0.0F;
	}

	@Override
	public float maxValue() {
		// Upper bound (conservative): amplitude
		return this.amplitude;
	}

	@Override
	public Noise mapAll(Visitor visitor) {
		return visitor.apply(new MountainRange(this.axis.mapAll(visitor), this.ridge.mapAll(visitor),
				this.invScale, this.axisScale, this.axisSpread, this.halfWidth,
				this.ridgeFreq, this.amplitude, this.sharpness));
	}

	@Override
	public Codec<MountainRange> codec() {
		return CODEC;
	}
}
