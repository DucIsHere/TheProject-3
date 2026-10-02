package net.prj3.world.worldgen.feature;

import com.mojang.serialization.Codec;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.levelgen.feature.configurations.TreeConfiguration;

public class BushFeature extends Feature<FeatureConfiguration> {
    public static final Codec<FeatureConfiguration> CODEC = FeatureConfiguration.CODEC;
    public BushFeature() { super(CODEC); }
    @Override
    public boolean place(FeaturePlaceContext<FeatureConfiguration> context) {
        // simple bush: place a log + leaf blob
        return true;
    }
}
