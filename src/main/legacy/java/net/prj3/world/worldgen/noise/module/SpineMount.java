package net.prj3.world.worldgen.noise.module;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.prj3.world.worldgen.noise.Noise;

public record LinkedEverest(
    Noise baseRange,   // Dãy núi Part 1
    Noise peakNoise,   // Noise tạo đỉnh Everest
    float peakMin,     // 1700.0f
    float peakMax      // 1800.0f
) implements Noise {
    public static final Codec<LinkedEverest> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        Noise.HOLDER_HELPER_CODEC.fieldOf("base_range").forGetter(LinkedEverest::baseRange),
        Noise.HOLDER_HELPER_CODEC.fieldOf("peak_noise").forGetter(LinkedEverest::peakNoise),
        Codec.FLOAT.fieldOf("peak_min").forGetter(LinkedEverest::peakMin),
        Codec.FLOAT.fieldOf("peak_max").forGetter(LinkedEverest::peakMax)
    ).apply(instance, LinkedEverest::new));

    @Override
    public float compute(float x, float z, int seed) {
        // 1. Lấy độ cao của dãy núi Part 1
        float rangeH = this.baseRange.compute(x, z, seed);

        // 2. Nếu không có núi (đồng bằng), trả về 0 luôn cho nhẹ CPU
        if (rangeH <= 0.0f) return 0.0f;

        // 3. Tính toán đỉnh Everest
        float rawPeak = this.peakNoise.compute(x, z, seed);
        // Chuyển [-1, 1] về [0, 1]
        float p = (rawPeak + 1.0f) / 2.0f;
        float peakH = peakMin + (p * (peakMax - peakMin));

        // 4. LINKED: Everest mọc đè lên dãy núi
        return Math.max(rangeH, peakH);
    }

    @Override public float minValue() {
        return 0.0f;
    }

    @Override public float maxValue() {
        return peakMax;
    }

    @Override public Codec<LinkedEverest> codec() {
        return CODEC;
    }
    
    @Override public Noise mapAll(Visitor v) { 
        return v.apply(new LinkedEverest(baseRange.mapAll(v), peakNoise.mapAll(v), peakMin, peakMax)); 
    }
}