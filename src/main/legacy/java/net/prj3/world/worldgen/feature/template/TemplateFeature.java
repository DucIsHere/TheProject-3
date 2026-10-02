package net.prj3.world.worldgen.feature.template;

import com.mojang.serialization.Codec;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.WorldGenLevel;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;
import net.prj3.world.worldgen.feature.template.paste.PasteConfig;

public class TemplateFeature extends Feature<TemplateFeature.Config> {
    public static record Config(String templateId, PasteConfig paste) implements FeatureConfiguration {}

    public static final Codec<Config> CODEC = RecordCodecProvider.create(inst -> inst.group(
        Codec.STRING.fieldOf("template_id").forGetter(Config::templateId),
        PasteConfig.CODEC.fieldOf("paste").forGetter(Config::paste)
    ).apply(inst, Config::new));

    public TemplateFeature() { super(CODEC); }

    @Override
    public boolean place(FeaturePlaceContext<Config> context) {
        WorldGenLevel world = context.level();
        BlockPos pos = context.origin();
        Config cfg = context.config();
        // TODO: load StructureTemplate and paste using PasteConfig
        return true;
    }
}
