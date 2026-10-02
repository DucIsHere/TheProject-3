package net.prj3.world.worldgen.noise.module;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.minecraft.util.Mth;

public record PeakLimiterNoise(Noise input, float minPeak, float maxPeak) implements Noise {
    public static final Codec<PeakLimiterNoise> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        Noise.CODEC.fieldOf("input").forGetter(PeakLimiterNoise::input),
        Codec.FLOAT.fieldOf("min_peak").forGetter(PeakLimiterNoise::minPeak), // 1700
        Codec.FLOAT.fieldOf("max_peak").forGetter(PeakLimiterNoise::maxPeak)  // 1800
    ).apply(instance, PeakLimiterNoise::new));

    @Override
    public float compute(float x, float z, int seed) {
        float v = this.input.compute(x, z, seed);
        
        // Nếu cao độ bắt đầu vượt quá 1700
        if (v > this.minPeak) {
            float delta = v - this.minPeak;
            float range = this.maxPeak - this.minPeak;
            
            // Sử dụng hàm nội suy để nén độ cao tiệm cận 1800
            // Công thức: min + (range * (delta / (delta + soft_factor)))
            float softFactor = range * 0.5f; 
            float squash = (delta / (delta + softFactor)) * range;
            
            return this.minPeak + squash;
        }
        
        return v;
    }

    @Override
    public float minValue() { return input.minValue(); }
    @Override
    public float maxValue() { return maxPeak; }
    @Override
    public Noise mapAll(Visitor visitor) { return new PeakLimiterNoise(visitor.apply(input), minPeak, maxPeak); }
    @Override
    public Codec<? extends Noise> codec() { return CODEC; }
}