package net.prj3.world.worldgen.feature;

import com.mojang.serialization.Codec;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;

public class SwampSurfaceFeature extends Feature<FeatureConfiguration> {
    public SwampSurfaceFeature() { super(FeatureConfiguration.CODEC); }
    @Override
    public boolean place(FeaturePlaceContext<FeatureConfiguration> context) {
        // place swamp specific surface
        return true;
    }
}
