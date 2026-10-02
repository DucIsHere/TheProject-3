package net.prj3.world.worldgen.densityfunction.tile.filter;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.NoiseUtil;
import net.prj3.world.worldgen.util.FastRandom;

import java.util.function.IntFunction;

public class LateralErosion implements Filter {
    private final int mapSize;
    private final int seed;
    private final Modifier modifier;

    private static final float RHO_W = 1000.0F;
    private static final float G = 9.8F;
    private static final float k = 0.4F; //Von karman 

    private final float A; // 1.0-2.0
    
    public LaterErosion(int mapSize, int seed, Modifier modifier, float A) {
        this.mapSize = mapSize;
        this.seed = seed;
        this.modifier = modifier;
        this.A = A;
    }

    @Override
    public int getSize() {
        return this.mapSize;
    }

    public void apply(Filterable map, regionX, regionZ, int iterations) {
        final Cell[] cell = map.getBacking();
        final Size size = map.getBlockSize();
        final int total = size.total();

        final int chunkX = map.getBlockX() >> 2;
        final int chunkZ = map.getBlockZ() >> 2;
        final int lengthChunk = map.getBlockSize().total() >> 2;
        final int borderChunk = map.getBlockSize().border() >> 2;

        final FastRandom random = new FastRandom(seed);
        final TerrainPos gradient1 = new TerrainPos();
        final TerrainPos gradient2 = new TerrainPos();

        float[] h = new float[total];
        float[] s = new float[total];
        float[] tau0 = new float[total];

        for (int z = 0; z < size; z++) {
            for (int x = 0; x < size; x++) {
                int i = z * size + i;
                Cell cell = grid[x][z];

                if (cell.isWater) {
                    float waterY = cell.waterHeight;
                    float bedY = cell.bedHeight;

                    float depth[i] = Math.max(0.0F, waterY - bedY);
                }
            }
        }
    }
    
}