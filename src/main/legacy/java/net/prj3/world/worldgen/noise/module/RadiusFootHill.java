package net.prj3.world.worldgen.noise.module;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.minecraft.util.Mth;

/**
 * Tạo độ dốc mượt mà cho chân núi trong bán kính xác định
 */
public record RadiusFoothill(Noise mountainNoise, float startRadius, float transitionWidth) implements Noise {
    public static final Codec<RadiusFoothill> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        Noise.CODEC.fieldOf("mountain_noise").forGetter(RadiusFoothill::mountainNoise),
        Codec.FLOAT.fieldOf("start_radius").forGetter(RadiusFoothill::startRadius), // Khoảng cách bắt đầu dốc
        Codec.FLOAT.fieldOf("transition_width").forGetter(RadiusFoothill::transitionWidth) // Độ rộng dải thoải (250)
    ).apply(instance, RadiusFoothill::new));

    @Override
    public float compute(float x, float z, int seed) {
        float baseHeight = this.mountainNoise.compute(x, z, seed);
        
        // Tính toán khoảng cách (hoặc dùng giá trị noise gốc để xác định rìa)
        // Với địa hình procedural, ta thường dùng giá trị noise làm mốc khoảng cách
        // Giả sử baseHeight = 0 là rìa núi
        
        // Logic: Nếu độ cao < 0.2 (vùng chân núi), ta áp dụng hàm Smoothstep để làm thoải
        float threshold = 0.2f; // Ngưỡng bắt đầu uốn chân núi
        if (baseHeight <= 0) return 0;
        if (baseHeight >= threshold) return baseHeight;

        // Nội suy mượt trong bán kính chuyển tiếp
        float alpha = baseHeight / threshold;
        float smoothAlpha = alpha * alpha * (3 - 2 * alpha); // Smoothstep

        return baseHeight * smoothAlpha;
    }

    @Override
    public float minValue() { return 0; }
    @Override
    public float maxValue() { return mountainNoise.maxValue(); }
    @Override
    public Noise mapAll(Visitor visitor) { return new RadiusFoothillNoise(visitor.apply(mountainNoise), startRadius, transitionWidth); }
    @Override
    public Codec<? extends Noise> codec() { return CODEC; }
}