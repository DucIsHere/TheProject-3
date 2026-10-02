package net.prj3.data.worldgen.preset.settings;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;

public class FeatureSettings {

    public static final Codec<FeatureSettings> CODEC = RecordCodecBuilder.create(i -> i.group(
            Codec.BOOL.fieldOf("trees").forGetter(f -> f.trees),
            Codec.BOOL.fieldOf("lakes").forGetter(f -> f.lakes),
            Codec.BOOL.fieldOf("caves").forGetter(f -> f.caves),
            Codec.BOOL.fieldOf("ores").forGetter(f -> f.ores)
    ).apply(i, FeatureSettings::new));

    public final boolean trees;
    public final boolean lakes;
    public final boolean caves;
    public final boolean ores;

    public FeatureSettings(boolean t, boolean l, boolean c, boolean o) {
        this.trees = t;
        this.lakes = l;
        this.caves = c;
        this.ores = o;
    }
}
