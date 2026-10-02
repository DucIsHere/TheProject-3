package net.prj3.world.worldgen.feature.template.decorator;

import net.minecraft.core.BlockPos;
import net.minecraft.world.level.LevelAccessor;

public class TreeContext {
    public final LevelAccessor level;
    public final BlockPos pos;

    public TreeContext(LevelAccessor level, BlockPos pos) {
        this.level = level; this.pos = pos;
    }
}
