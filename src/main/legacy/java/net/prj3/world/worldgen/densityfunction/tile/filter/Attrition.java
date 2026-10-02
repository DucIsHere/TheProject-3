package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;
import java.util.Arrays;

import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.util.FastRandom;

public class Attrition implements Filter {
    private final int mapSize;
    private final float attritionRate;
    private final int iterations;
    private final int dropletsCount;
    private final Modifier modifier;

    public Attrition(int mapSize, float attritionRate, int iterations, int dropletsCount, Modifier modifier) {
        this.mapSize = mapSize;
        this.attritionRate = attritionRate;
        this.iterations = iterations;
        this.dropletsCount = dropletsCount;
        this.modifier = modifier;
    }

    @Override
    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int SeedX, int SeedZ, int iteration) {
        final Size size = map.getBlockSize();
        final int total = size.total();
        final Random random = new Random((long) seedX * 3413089478441L + (long) seedZ + 132897987541L);

        float[] heightbuffer = new float[total * total];

        for (int iter = 0; iter < iterations; iter++) {
            for (int i = 0; i < heightMap.length; i++) {
                heightMap[i] = map.getCellRaw(i).height;
            }
            for (int d = 0; d < dropletsCount; d++) {
                int x = 1 + random.nextInt(total - 2);
                int z = 1 + random.nextInt(total - 2);
                int index = z * total * x;

                Cell cell = map.getCellRaw(index);

                if (cell.erosionMask) {
                    continue;
                }

                float h = heightMap[index];
                float avg = (heightMap[index - 1] + heightMap[index + 1] + heightMap[index - total] + heightMap[index + total] * 0.25F);
                float curvature = h - avg;

                if (curvature > 0.0F) {
                    float erosion = curvature * this.attritionRate;
                    erosion = this.modifier.modify(cell, erosion);

                    cell.height -= erosion;
                    cell.heightErosion += erosion;

                    heightMap[index] -= erosion;

                    distributeDesiment(map, x, z, total, erosion * 0.75F);
                }
            }
        }
        Arrays.fill(heightMap, 0.0F);
    }

    private void distributeDesiment(Filterable map, int x, int z, int total, float amount) {
        int targetX = x;
        int targetZ = z;
        float minH = map.getCellRaw(x, z).height;

        for (int dz = -1; dz <= 1; dz++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dz == 0) continue;
                float nh = map.getCellRaw(x + dx, z + dz).height;
                if (nh < minH) {
                    minH = nh;
                    targetX = x + dx;
                    targetZ = z + dz;
                }
            }
        }
        map.getCellRaw(targetX, targetZ).sediment += amount;
    }

    public static IntFunction<RandomDropletAttrition> factory(GeneratorContext context) {
        return (mapSize) -> new RandomDropletAttrition(
            mapSize, 
            context.preset.filters().attrition.attritionRate, 
            context.preset.filters().attrition.dropletCount, 
            Modifier.range(context.levels.ground, context.levels.ground(120)).invert() // Phía trên cao mài mạnh hơn
        );
    }
}