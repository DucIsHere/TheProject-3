package net.prj3.data.worldgen;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.levelgen.synth.NormalNoise;

public class RGFRiverConfigured {
    public final int width;
    public final int depth;
    public final double scale;
    public final double riverSize;
    public final BlockState floor;
    public final Heightmap.Types heightmap;
    public final NormalNoise riverNoise;

    public static final Codec<RiverConfig> CODEC = RecordCodecBuilder.create(i ->
        i.group(
            Codec.INT.fieldOf("width").forGetter(v -> v.width),
            Codec.INT.fieldOf("depth").forGetter(v -> v.depth),
            Codec.DOUBLE.fieldOf("scale").forGetter(v -> v.scale),
            Codec.DOUBLE.fieldOf("river_size").forGetter(v -> v.riverSize),
            BlockState.CODEC.fieldOf("floor_block").forGetter(v -> v.floor),
            Heightmap.Types.CODEC.fieldOf("heightmap").forGetter(v -> v.heightmap),
            NormalNoise.CODEC.fieldOf("river_noise").forGetter(v -> v.riverNoise)
        ).apply(i, RiverConfig::new)
    );

    public RiverConfig(int width, int depth, double scale, double riverSize,
                       BlockState floor, Heightmap.Types heightmap, NormalNoise noise) {
        this.width = width;
        this.depth = depth;
        this.scale = scale;
        this.riverSize = riverSize;
        this.floor = floor;
        this.heightmap = heightmap;
        this.riverNoise = noise;
    }
}
