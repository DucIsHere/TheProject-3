package net.prj3.world.worldgen.surface.rule;

import java.util.List;

import com.google.common.collect.ImmutableList;

import com.mojang.serialization.Codec;

import net.minecraft.core.Holder;
import net.minecraft.core.HolderGetter;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.level.levelgen.SurfaceRules;
import net.prj3.platform.RegistryUtil;

import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.surface.rule.StrataRule.Strata;

// Giả định các class/biến này tồn tại trong project của bạn
// import com.regenerationforrged.world.worldgen.noise.Noises; 

public class RGFSurfaceRules {

    public static void bootstrap() {
        register("strata", StrataRule.CODEC);
        register("snow_strata", SnowStrataRule.CODEC);
        // Cần đăng ký thêm AdvancedSnowStrataRule nếu bạn sử dụng nó
        // register("advanced_snow_strata", AdvancedSnowStrataRule.CODEC);
    }

    public static StrataRule strata(ResourceLocation name, Holder<Noise> selector, List<Strata> strata, int iterations) {
        return new StrataRule(name, selector, strata, iterations);
    }

    public static SurfaceRules.RuleSource snowStrata(Holder<Noise> depth, Holder<Noise> layer) {
        return new SnowStrataRule(depth, layer);
    }

    public static void register(String name, Codec<? extends SurfaceRules.RuleSource> value) {
        RegistryUtil.register(BuiltInRegistries.MATERIAL_RULE, name, value);
    }

    // --- Các phương thức bổ trợ cho Logic Worldgen ---

    private static SurfaceRules.RuleSource makeSnowRule(HolderGetter<Noise> noise) {
        // Lưu ý: Đảm bảo AdvancedSnowStrataRule đã được định nghĩa
        return new AdvancedSnowStrataRule(
            noise.getOrThrow(null /* Noises.SNOW_DEPTH */), 
            noise.getOrThrow(null /* Noises.SNOW_DRIFT */)
        );
    }

    public static SurfaceRules.RuleSource overworld(HolderGetter<Noise> noise, boolean useStrata) {
        ImmutableList.Builder<SurfaceRules.RuleSource> builder = ImmutableList.builder();

        // 1. Tuyết được ưu tiên xử lý trước (đặt lên trên)
        builder.add(makeSnowRule(noise));

        // 2. Sau đó mới đến các lớp địa tầng (Strata)
        if (useStrata) {
            // builder.add(makeStrataRule(noise)); // Cần đảm bảo hàm makeStrataRule tồn tại
        }

        return SurfaceRules.sequence(builder.build().toArray(SurfaceRules.RuleSource[]::new));
    }
}