package net.prj3.world.worldgen.feature;

import com.mojang.serialization.Codec;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;

public class DecorateSnowFeature extends Feature<FeatureConfiguration> {
    public DecorateSnowFeature() { super(FeatureConfiguration.CODEC); }
    @Override
    public boolean place(FeaturePlaceContext<FeatureConfiguration> context) {
        // add snow decoration based on settings
        return true;
    }
}
