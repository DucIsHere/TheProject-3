package net.prj3.world.worldgen.feature;

import com.mojang.serialization.Codec;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;

public class ErodeFeature extends Feature<FeatureConfiguration> {
    public ErodeFeature() { super(FeatureConfiguration.CODEC); }
    @Override
    public boolean place(FeaturePlaceContext<FeatureConfiguration> context) {
        // apply small erosion/strata adjustments
        return true;
    }
}
