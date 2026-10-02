package net.prj3.data.worldgen;

import net.minecraft.core.HolderGetter;
import net.minecraft.data.worldgen.BootstapContext;
import net.minecraft.data.worldgen.placement.PlacementUtils;
import net.minecraft.resources.ResourceKey;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.level.levelgen.feature.ConfiguredFeature;
import net.minecraft.world.level.levelgen.placement.PlacedFeature;

public class RGFRiverPlacedFeatures {

    public static final ResourceKey<PlacedFeature> RIVER = createKey("river");

    public static void bootstrap(BootstapContext<PlacedFeature> ctx) {
        HolderGetter<ConfiguredFeature<?, ?>> features = ctx.lookup(net.minecraft.core.registries.Registries.CONFIGURED_FEATURE);
        PlacementUtils.register(ctx, RIVER, features.getOrThrow(RGFRiverConfiguredFeatures.RIVER));
    }

    private static ResourceKey<PlacedFeature> createKey(String name) {
        return ResourceKey.create(net.minecraft.core.registries.Registries.PLACED_FEATURE, new ResourceLocation("rgf", name));
    }
}
