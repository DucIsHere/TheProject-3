package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;
import java.util.Arrays;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import con.regenerationforrged.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.densityfunction.Size;
import net.prj3.world.worldgen.cell.heightmap.Levels;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

public class BankErosion implements Filter {
    private final int mapSize;
    private final int seed;
    private final Modifier modifier;

    private final float RHO_WATER = 1000.0F;
    private final float GRAVITY = 9.81F;

    private final float erodbilitySpeedKd = 0.02F;
    private final float criticalShearStress = 2.0F;
    private final float maxStableSlope = 0.45F;
    private final float slumpingCoeff = 0.3F;
    private final float sedimentConvertRate = 0.5F;

    private final float riverWaterLevel;

    private final int[][] erosionBushIndices;
    private final float[][] erosionBushWeights;

    public BankErosion(int mapSize, Modifier modifier, float erodbilityKd, float criticalShearStress, float maxStableSlope, float slumpingCoeff, float sedimentConvertRate, float riverWaterLevel, float angleOfReposeSlope) {
        this.mapSize = mapSize;
        this.modifier = modifier;
        this.erodbilityKd = erodbilityKd;
        this.criticalShearStress = criticalShearStress;
        this.maxStableSlope = maxStableSlope;
        this.slumpingCoeff = slumpingCoeff;
        this.slumpRate = slumpRate;
        this.sedimentConvertRate = sedimentConvertRate;
        this.riverWaterLevel = riverWaterLevel;
        this.riverWaterLevel = riverWaterLevel;

        this.erosionBushIndices = new int[mapSize * mapSize][];
        this.erosionBushWeights = new float[mapSize * mapSize][];

        this.initErosionBushes(3);
    }

    @Override 
    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int SeedX, int SeedZ, int iteration) {
        final Size size = map.getBlockSize();
        Cell[] cell = map.getBacking();
        int total = size.total();
        
        float[] fluidDepthMap = new float[total];
        float[] slopeMap = new float[total];
        float[] erosionOutput = new float[total];

        for (int x = 0; x < side.width() - 1; x++) {
            for (int z = 0; z < side.height() - 1; z++) {
                int idx = size.idx(x, z);
                float currentH = cell[idx].height;

                if (currentH >= riverWaterLevel - 3.0F && currentH <= riverWaterLevel + 14.0F) {
                    fluidDepthMap[idx] = Math.max(0.0, (riverWaterLevel + 0.5F) - currentH);
                    
                    float dzdx = (cell[size.idx(x + 1, z)].height - cell[size.idx(x - 1, z)].height) / 2.0F;
                    float dzdz = (cell[size.idx(x, z + 1)].height - cell[size.idx(x, z - 1)].height) / 2.0F;
                    slopeMap[idx] = (float) Math.sqrt(dzdx * dzdx + dzdz * dzdz);
                }
            }
        }

        for (int x = 2; x < side.width() - 2; x++) {
            for (int z = 2; z < side.height() - 2; z++) {
                int idx = size.idx(x, z);
                float slope = slopeMap[idx];
                float waterDepth = fluidDepthMap[idx];

                if (slope <= 0.0F) continue;

                if (waterDepth > 0.0F) {
                    float shearStress = RHO_WATER * GRAVITY * waterDepth * slope * 0.00005F;

                    if (shearStress > criticalShearStress) {
                        erosionOutput[idx] = erodbilitySpeedKd * (shearStress - criticalShearStress);
                    }
                }

                if (cell[idx].height > riverWaterLevel && slope > maxStableSlope) {
                    float overSlope = slope - maxStableSlope;
                    erosionOutput[idx] += slumpingCoeff * overSlope * 0.1F;
                }

                float totalErosion = erosionOutput[idx];
                
                if (totalErosion > 0.0F && erosionBushIndices != null) {
                    float dzdx = (cell[size.idx(x + 1, z)].height - cell[size.idx(x - 1, z)].height);
                    float dzdz = (cell[size.idx(x, z + 1)].height - cell[size.idx(x, z - 1)].height);

                    int moveX = dxdz > 0 ? -1 : 1;
                    int moveZ = dzdz > 0 ? -1 : 1;

                    int riverIdx = size.index(x + moveX, z + moveZ);

                    for (int b = 0; b < erosionBushIndices[idx].length; b++) {
                        int bushIdx = erosionBushIndices[idx][b];
                        float weight = erosionBushWeights[idx][b];

                        float actualChange = modifier.modify(cells[bushIdx], totalErosion * weight);

                        cells[bushIdx].height -= actualChange;
                        cells[bushIdx].heightErosion -= actualChange;

                        if (cells[riverIdx].height < riverWaterLevel) {
                            cells[riverIdx].height += actualChange * 0.4f; // Bồi tụ lòng sông
                            cells[riverIdx].sediment += actualChange * 0.4f;
                        }
                    }
                }
            }
        }
    }

    private void initErosionBushes(int radius) {
        for (int i = 0; i < erosionBushIndices.length; i++) {
            int cx = i % mapSize;
            int cz = i / mapSize;

            int[] tempIndices = new int[radius * radius * 4];
            float[] tempWeight = new float[radius * radius * 4];

            int count = 0;
            float sum = 0;

            for (int rz = -radius; rz <= radius; rz++) {
                for (int rx = -radius; rx <= radius; rx++) {
                    float distSq = rx * rx + rz * rz;
                    if (distSq < radius * radius) {
                        int tx = cx + rx;
                        int tz = cz + rz;
                        if (tx >= 0 && tx < mapSize && tz >= 0 && tz < mapSize) {
                            float d = (float) Math.sqrt(distSq) / radius;
                            float w = 1.0f - d; // Falloff tuyến tính khoét vách dốc gọn gàng
                            tempIndices[count] = tz * mapSize + tx;
                            tempWeights[count] = w;
                            sum += w;
                            count++;
                        }
                    }
                }
            }
            erosionBrushIndices[i] = Arrays.copyOf(tempIndices, count);
            erosionBrushWeights[i] = new float[count];
            for (int j = 0; j < count; j++) {
                erosionBrushWeights[i][j] = tempWeights[j] / sum;
            }
        }
    }

    public static class Factory implements IntFunction<BankErosion> {
        private static final int SEED_OFFSET = 456789;
        private final int seed;
        private final Modifier modifier;
        private final float waterLevel;

        public Factory(final int seed, final FilterSettings filter, final Levels levels) {
            this.seed = seed + SEED_OFFSET;
            this.modifier = new Modifier.range(waterLevel - 4, levels.ground(130)).invert();
            this.waterLevel = (float) levels.water;
        }

        @Override
        public BankErosion apply(final int mapSize) {
            return new BankErosion(this.seed, mapSize, this.modifier, this.waterLevel);
        }
    }
}