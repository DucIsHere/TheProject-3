package net.prj3.world.worldgen.feature.template.paste;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;

public class PasteConfig {
    public final int baseExtension;
    public final boolean overwrite; // example

    public static final Codec<PasteConfig> CODEC = RecordCodecBuilder.create(inst -> inst.group(
        Codec.INT.fieldOf("base_extension").orElse(3).forGetter(c -> c.baseExtension),
        Codec.BOOL.fieldOf("overwrite").orElse(false).forGetter(c -> c.overwrite)
    ).apply(inst, PasteConfig::new));

    public PasteConfig(int baseExtension, boolean overwrite) {
        this.baseExtension = baseExtension;
        this.overwrite = overwrite;
    }
}
