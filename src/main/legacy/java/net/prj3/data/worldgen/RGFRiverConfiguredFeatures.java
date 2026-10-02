package net.prj3.data.worldgen;

import net.minecraft.core.HolderGetter;
import net.minecraft.core.registries.Registries;
import net.minecraft.data.worldgen.BootstapContext;
import net.minecraft.data.worldgen.placement.PlacementUtils;
import net.minecraft.resources.ResourceKey;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.level.levelgen.feature.ConfiguredFeature;
import net.minecraft.world.level.levelgen.placement.PlacedFeature;
import net.prj3.data.worldgen.RGFRiverFeature;
import net.prj3.data.worldgen.preset.RiverSettings;

public class RGFRiverConfiguredFeatures {

    public static final ResourceKey<ConfiguredFeature<?, ?>> RIVER = createKey("river");

    public static void bootstrap(BootstapContext<ConfiguredFeature<?, ?>> ctx) {
        RGFRiverSettings s = RGFRiverSettings.defaults();

        FeatureUtils.register(ctx, RIVER, new RGFRiverFeature(), new RGFRiverFeature.Config(s));
    }

    private static ResourceKey<ConfiguredFeature<?, ?>> createKey(String name) {
        return ResourceKey.create(Registries.CONFIGURED_FEATURE, new ResourceLocation("rgf", name));
    }
}
