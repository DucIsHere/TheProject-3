package net.prj3.data.worldgen;

import com.mojang.serialization.Codec;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.WorldGenLevel;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.levelgen.feature.Feature;
import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;

public class RGFRiverFeatures extends Feature<RiverConfig> {

    public RGFRiverFeatures(Codec<RiverConfig> codec) {
        super(codec);
    }

    @Override
    public boolean place(FeaturePlaceContext<RiverConfig> ctx) {
        WorldGenLevel level = ctx.level();
        RiverConfig cfg = ctx.config();
        BlockPos pos = ctx.origin();

        int chunkX = pos.getX() >> 4;
        int chunkZ = pos.getZ() >> 4;

        ChunkAccess chunk = level.getChunk(chunkX, chunkZ);

        // noise mask (đường river chạy theo noise)
        double baseMask = cfg.riverNoise.noise(pos.getX() * cfg.scale, pos.getZ() * cfg.scale);

        // nếu mask > threshold → ko đặt river
        if (Math.abs(baseMask) > cfg.riverSize)
            return false;

        // chiều rộng half width
        int half = cfg.width / 2;

        for (int x = -half; x <= half; x++) {
            for (int z = -half; z <= half; z++) {

                int wx = pos.getX() + x;
                int wz = pos.getZ() + z;

                BlockPos.MutableBlockPos mp = new BlockPos.MutableBlockPos(wx, 0, wz);

                int h = chunk.getHeight(cfg.heightmap, wx & 15, wz & 15);

                int depth = cfg.depth;

                // carve river xuống
                for (int y = h; y > h - depth; y--) {
                    mp.setY(y);
                    level.setBlock(mp, Blocks.WATER.defaultBlockState(), 2);
                }

                // bottom (đá sông)
                mp.setY(h - depth);
                level.setBlock(mp, cfg.floor, 2);
            }
        }

        return true;
    }
}
