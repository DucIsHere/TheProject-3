package net.prj3.data.world.feature;

import net.minecraft.core.registries.Registries;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.fabricmc.fabric.api.event.registry.FabricRegistryBuilder;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.core.Registry;

public class RGFFeatures {

    public static final Registry<Feature<?>> REGISTRY =
            FabricRegistryBuilder.createSimple(Registries.FEATURE, id("features")).buildAndRegister();

    public static final Feature<RiverConfig> RGF_RIVER =
            register("rgf_river", new RGFRiverFeature(RiverConfig.CODEC));

    private static <T extends Feature<?>> T register(String id, T feature) {
        Registry.register(REGISTRY, id(id), feature);
        return feature;
    }

    private static ResourceLocation id(String s) {
        return new ResourceLocation("yourmodid", s);
    }
}
