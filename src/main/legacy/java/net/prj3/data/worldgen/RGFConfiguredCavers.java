package net.prj3.data.worldgen;

import net.minecraft.core.registries.Registries;
import net.minecraft.data.worldgen.BootstapContext;
import net.minecraft.resources.ResourceKey;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.level.levelgen.carver.ConfiguredWorldCarver;
import net.minecraft.world.level.levelgen.carver.WorldCarver;

import net.prj3.data.worldgen.preset.CaveSettings;

public class RGFConfiguredCarvers {

    public static final ResourceKey<ConfiguredWorldCarver<?>> CAVES =
            createKey("caves");

    public static void bootstrap(BootstapContext<ConfiguredWorldCarver<?>> ctx) {

        // lấy default từ presets
        CaveSettings settings = CaveSettings.defaults();

        ctx.register(
                CAVES,
                new ConfiguredWorldCarver<>(
                        WorldCarver.CAVE,
                        settings.toConfig()
                )
        );
    }

    private static ResourceKey<ConfiguredWorldCarver<?>> createKey(String name) {
        return ResourceKey.create(
                Registries.CONFIGURED_CARVER,
                new ResourceLocation("rgf", name)
        );
    }
}
