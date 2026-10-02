package net.prj3.data.worldgen;

import net.minecraft.registry.RegistryKey;
import net.minecraft.registry.RegistryKeys;
import net.minecraft.registry.HolderGetter;
import net.minecraft.registry.entry.RegistryEntry;

import net.minecraft.data.server.feature.PlacedFeatureBootstrap;
import net.minecraft.data.server.worldgen.BootstrapContext;
import net.minecraft.util.Identifier;

import net.minecraft.world.gen.feature.ConfiguredFeature;
import net.minecraft.world.gen.feature.PlacedFeature;
import net.minecraft.world.gen.feature.PlacementModifier;
import net.minecraft.world.gen.feature.PlacementModifiers;
import net.minecraft.world.gen.feature.CountPlacementModifier;
import net.minecraft.world.gen.feature.HeightmapPlacementModifier;

import net.minecraft.world.gen.feature.FeaturePlacement;

import net.minecraft.world.gen.GenerationStep;
import net.minecraft.world.level.levelgen.feature.FeaturePlaced;

import net.prj3.RGFCommon;
import net.prj3.data.worldgen.preset.MiscellaneousSettings;
import net.prj3.data.worldgen.preset.Preset;
import net.prj3.data.worldgen.RGFConfiguredFeatures;

public class RGFPlacedFeatures {

    public static final RegistryKey<PlacedFeature> GIANT_RED_MUSHROOM = create("trees/giant_red_mushroom");
    public static final RegistryKey<PlacedFeature> GIANT_BROWN_MUSHROOM = create("trees/giant_brown_mushroom");

    public static final RegistryKey<PlacedFeature> OAK_TREES = create("trees/oak");
    public static final RegistryKey<PlacedFeature> BIRCH_TREES = create("trees/birch");
    public static final RegistryKey<PlacedFeature> JUNGLE_TREES = create("trees/jungle");
    public static final RegistryKey<PlacedFeature> ACACIA_TREES = create("trees/acacia");
    public static final RegistryKey<PlacedFeature> DARK_OAK_TREES = create("trees/dark_oak");

    public static void bootstrap(Preset preset, BootstrapContext<PlacedFeature> ctx) {

        HolderGetter<ConfiguredFeature<?, ?>> features = ctx.getRegistryLookup(RegistryKeys.CONFIGURED_FEATURE);
        MiscellaneousSettings miscellaneousSettings = preset.misc();

        // Tree placement modifiers
        PlacementModifier heightmap = HeightmapPlacementModifier.of();
        PlacementModifier count5 = CountPlacementModifier.of(5);
        PlacementModifier count7 = CountPlacementModifier.of(7);

        // ===== Mushroom =====
        ctx.register(
            GIANT_RED_MUSHROOM,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.GIANT_RED_MUSHROOM), 
                PlacementModifiers.list(heightmap, count5))
        );

        ctx.register(
            GIANT_BROWN_MUSHROOM,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.GIANT_BROWN_MUSHROOM), 
                PlacementModifiers.list(heightmap, count5))
        );

        // ===== Trees =====
        ctx.register(
            OAK_TREES,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.OAK_TREE),
                PlacementModifiers.list(heightmap, count7))
        );

        ctx.register(
            BIRCH_TREES,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.BIRCH_TREE),
                PlacementModifiers.list(heightmap, count7))
        );

        ctx.register(
            JUNGLE_TREES,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.JUNGLE_TREE),
                PlacementModifiers.list(heightmap, count5))
        );

        ctx.register(
            ACACIA_TREES,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.ACACIA_TREE),
                PlacementModifiers.list(heightmap, count5))
        );

        ctx.register(
            DARK_OAK_TREES,
            new PlacedFeature(features.getOrThrow(RGFConfiguredFeatures.DARK_OAK_TREE),
                PlacementModifiers.list(heightmap, count5))
        );

        context.register(
            Carvers.CAVE,
            new PlacesCavers(RGFCongiguredCavers.CAVE)
        );
    }

    private static RegistryKey<PlacedFeature> create(String id) {
        return RegistryKey.of(RegistryKeys.PLACED_FEATURE, RGFCommon.id(id));
    }

    public static final ResourceKey<PlacedFeature> RIVER_PLACED =
            ResourceKey.create(Regiatries.PLACED_FEATURE, id("river"));

    public static void bootstrap(BootstrapContext<PlacedFeature> ctx) {

        HolderGetter<ConfiguredFeature<?, ?>> configured =
            ctx.lookup(Registries.CONFIGURED_FEATURE);

        PlacementUtils.register(
            ctx,
            RIVER_PLACED,
            configured.getOrThrow(RGFConfiguredFeatures.RIVER),
            PlacementUtils.FULL_RANGE
        );
    }
}
