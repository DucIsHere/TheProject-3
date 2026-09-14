package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.IntFunction;
import java.util.function.Arrays;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.heightmap.Levels;
import net.prj3.world.worldgen.util.FastRandom;
import net.prj3.world.worldgen.noise.NoiseUtil;

public class FreezeThaw extends Filter {
    private final int mapSize;
    private final int seed;
    private final Modifier modifier;

    private static final float pW = 1000.0F; //density of water 1000kg/m^3 at 4degrees
    private static final float pI = 987.0F; //density of ice 987kg/m^3 at 0 degrees
    private static final float Lf = 334000.0F; //latent heat of fusion for water in J/kg
    private static final float cW = 4184.0F; //specific heat capacity of water in J/kg * K
    private static final float cI = 2108.9F; //specific heat capacity of ice in J/kg * K
    private static final float kW = 0.56F; //thermal conductivity of water in W/m * K
    private static final float kI = 2.22F; //thermal conductivity of ice in W/m * K
    private static final float To = 273.15F; //reference temperature in Kelvin (0 degrees Celsius)
    private static final float freezeMaxDepth = 3.0F; //maxium depth of freezing in meters

    private final float Porosity;
    private final float tensileStrength;
    private final float criticalSaturation;
    private final float softeningFactor;
    private final float clapeyronFactor = 1.11F;
    private final float freezeThawCycles; // number of freeze-thaw cycle to simulate
    private final float mExponent = 2.0F; //exponent for moisture retention curve
    private final float nExponent = 1.2F; //exponent for moisture retention curve

    private final float breakAmount;
    private final float zAmountDepth;
    private final float dDampingDepth;
    private final float snowLine = 0.5F; // Bổ sung hằng số độ cao tuyết giả định
    private final float sedimentCapacity = 0.02F; // Bổ sung sức chứa trầm tích giả định

    private final int[][] erosionBrushIndices;
    private final int[][] erosionBrushWeights;

    public FreezeThaw(int mapSize, int seed, Modifier modifier, float porosity, float tensileStrength, float criticalSaturation, float softeningFactor, float calpeyronFactor, float freezeThawCycles, float breakAmount, float zAmountDepth, float dDampingDepth) {
        this.mapSize = mapSize;
        this.seed = seed;
        this.modifier = modifier;
        this.Porosity = porosity;
        this.tensileStrength = tensileStrength;
        this.criticalSaturation = criticalSaturation;
        this.softeningFactor = softeningFactor;
        this.clapeyronFactor = 1.11F;
        this.freezeThawCycles = freezeThawCycles;
        this.breakAmount = breakAmount;
        this.zAmountDepth = zAmountDepth;
        this.dDampingDepth = dDampingDepth;
        this.erosionBrushIndices = new int[mapSize * mapSize][];
        this.erosionBrushWeights = new int[mapSize * mapSize][];
        this.initBrushes(mapSize, 4);
    }

    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int regionX, int regionZ, int iterations) {
        final Size size = map.getBlockSize();
        final Cell[] cell = map.getBacking();

        final int width = size.width();
        final int height = size.height();
        final int currentMapSize = size.total();
        final int maxPos = (float)(currentMapSize - 2);
        final int chunkX = map.getBlockX() >> 3;
        final int chunkZ = map.getBlockZ() >> 3;
        final int lengthChunk = map.getBlockSize().total() >> 3;
        final int borderChunk = map.getBlockSize().border() >> 3;

        final FastRandom random = new FastRandom(seed);
        final TerrainPos gradients1 = new TerrainPos();
        final TerrainPos gradients2 = new TerrainPos();

        float[] damage = new float[currentMapSize];
        float[] moisture = new float[currentMapSize];

        float[] temperature = new float[currentMapSize];
        float[] sediment = new float[currentMapSize];

        for (int i = 0; i < total; i++) {
            int x = i % width;
            int z = i / width;
            float noise = NoiseUtil.oerlin2D(x, z, seed, 0.5F);
            moistureMap[i] = Math.clamp(0.4F + (noise * 0.5F), 0.1F, 1.0F);
        }

        for (int i = 0; i < iterations; ++i) {
            final long iterationSeed = NoiseUtil.seed(this.seed + i);
            for (int cz = 0; cz < lengthChunk; ++cz) {
                final int relZ = cz << 3;
                final int seedZ = chunkZ + cz - borderChunk;
                for (int cx = 0; cx < lengthChunk; ++cx) {
                    final int relX = cx << 3;
                    final int seedX = chunkX + cx - borderChunk;
                    final long chunkSeed = NoiseUtil.seed(seedX, seedZ);
                    random.seed(chunkSeed, iterationSeed);

                    float posX = (float) (relX + random.nextInt(16));
                    float posZ = (float) (relZ + random.nextInt(16));

                    this.freezeThawCycle(posX, posZ, damageMap, moistureMap, tempMap, sedimentMap, gradients1, gradients2);
                }
            }
        } 
    }

    private void applyFreezeThawParticle(float posX, float posZ, float[] damageMap, float[] moistureMap, float[] tempMap, float[] sedimentMap, TerrainPos gradient1, TerrainPos gradient2) {
        float dirX = 0.0F;
        float dirZ = 0.0F;
        float sediment = 0.0F;

        gradient1.reset();
        gradient2.reset();

        for (int lifeTime = 0; lifeTime < this.freezeThawCycles; ++lifeTime) {
            final int nodeX = (int) posX;
            final int nodeZ = (int) posZ;

            final int idx = nodeZ * width + nodeX;

            if (idx < 0 || idx > currentMapSize || cell[idx].erosionMask) return;

            final float cellOfSetX = posX - nodeX;
            final float cellOfSetZ = posZ - nodeZ;

            gradient1.at(cell, width, posX, posZ);

            // The Fourier transform of a heat transfer process changes over time.
            float tMean = 8.0F - (gradient1.height - snowLine) * 0.35F;
            float omega = (float) (2 * Math.PI / this.freezeThawCycles);
            float zd = zAmountDepth / dDampingDepth;
            float damping = (float) Math.exp(-zd);
            float currentTemp = tMean + (14.0F * damping * (float) Math.cos(omega * lifeTime -zd));

            if (currentTemp < 0) {
                float pMaxTemp = -clapeyronFactor * currentTemp;
                float suctionPotential = pMaxTemp;
                float Kh = 0.04F * Porosity;
                float fluxQ = Kh * suctionPotential * (1.0 - moistureMap[idx]);
                moistureMap[idx] = Math.min(1.0F, moistureMap[idx] + fluxQ);
            }

            dirX = dirX * 0.1F - gradient1.gradientX * 0.5F;
            dirZ = dirZ * 0.1F - gradient1.gradientZ * 0.5F; // dirZ = dirZ * 0.1F - gradient1.gradientY * 0.5F;

            float len = (float) Math.sqrt(dirX * dirX + dirZ * dirZ);

            if (len > 0.0F) {
                dirX /= len;
                dirZ /= len;
            }

            posX += dirX;
            posZ += dirZ;

            if (posX < 1.0F || posX >= width - 1 || posZ < 1.0F || posZ >= width - 1) {
                return;
            }

            final float newHeight = gradient2.at(cell, weight, totalCells, posX, posZ);
            final float deltaHeight = newHeight - gradient1.height;

            float pMax = 0.0F;

            if (currentTemp < 0 && moistureMap[idx] > criticalSaturation) {
                pMax = (-clapeyronFactor * currentTemp) * moistureMap[idx];
            }

            if (pMax > 0.0F) {
                float pRatio = pMax / tensileStrength;

                if (pRatio > 0.1F) {
                    float currentD = damageMap[idx];
                    float oneMinusD = Math.max(0.01F, 1.0F - currentD);
                    float deltaD = (float) (Math.pow(pRatio, mExponent) * Math.pow(oneMinusD, -this.nExponent)) * 0.05F;
                }
            }

            if (damageMap[idx] >= 0.85F) {
                float amountToErode = Math.min(sedimentCapacity * this.softeningFactor, this.breakAmount);
                for (int b = 0; b < this.erosionBrushIndices[idx].length; ++b) {
                    int nodeIndex = this.erosionBrushIndices[idx][b];
                    float brushWeight = this.erosionBrushWeights[idx][b];
                    float weighedErodeAmount = amountToErode * brushWeight;
                    
                    if (!cells[nodeIndex].erosionMask) {
                        float deltaSediment = this.modifier.modify(cells[nodeIndex], weighedErodeAmount);
                        cells[nodeIndex].height -= deltaSediment;
                        cells[nodeIndex].heightErosion -= deltaSediment;
                        sediment += deltaSediment; // Gom đá mủn vỡ vào dòng sạt lở của hạt
                    }
                }
                damageMap[idx] = 0.05F;
                moistureMap[idx] *= 0.2F;
            }

            else if (sediment > sedimentCapacity || currentTemp > 1.5f) {
                float amountToDeposit = (sediment - sedimentCapacity) * 0.12f;
                sediment -= amountToDeposit;

                if (!cells[idx].erosionMask) {
                    float change = this.modifier.modify(cells[idx], amountToDeposit);
                    cells[idx].height += change;
                    cells[idx].sediment += change; // Đánh dấu bồi tụ sỏi đá sạt lở (Talus Scree)
                }
            }
        }
    }
    private void initBrushes(final int size, final int radius) {
        final int[] xOffsets = new int[radius * radius * 4];
        final int[] yOffsets = new int[radius * radius * 4];
        final float[] weights = new float[radius * radius * 4];
        float weightSum = 0.0f;
        int addIndex = 0;
        for (int i = 0; i < this.erosionBrushIndices.length; ++i) {
            final int centreX = i % size;
            final int centreY = i / size;
            if (centreY <= radius || centreY >= size - radius || centreX <= radius + 1 || centreX >= size - radius) {
                weightSum = 0.0f;
                addIndex = 0;
                for (int y = -radius; y <= radius; ++y) {
                    for (int x = -radius; x <= radius; ++x) {
                        final float sqrDst = (float)(x * x + y * y);
                        if (sqrDst < radius * radius) {
                            final int coordX = centreX + x;
                            final int coordY = centreY + y;
                            if (coordX >= 0 && coordX < size && coordY >= 0 && coordY < size) {
                                final float weight = 1.0f - (float)Math.sqrt(sqrDst) / radius;
                                weightSum += weight;
                                weights[addIndex] = weight;
                                xOffsets[addIndex] = x;
                                yOffsets[addIndex] = y;
                                ++addIndex;
                            }
                        }
                    }
                }
            }
            final int numEntries = addIndex;
            this.erosionBrushIndices[i] = new int[numEntries];
            this.erosionBrushWeights[i] = new float[numEntries];
            for (int j = 0; j < numEntries; ++j) {
                this.erosionBrushIndices[i][j] = (yOffsets[j] + centreY) * size + xOffsets[j] + centreX;
                this.erosionBrushWeights[i][j] = weights[j] / weightSum;
            }
        }
    }

    private static class TerrainPos {
        private float height;
        private float gradientX;
        private float gradientY;
        
        private TerrainPos at(final Cell[] nodes, final int width, final int totalCells, final float posX, final float posY) {
            final int coordX = (int) posX;
            final int coordY = (int) posY;
            final float x = posX - coordX;
            final float y = posY - coordY;
            final int nodeIndexNW = coordY * width + coordX;
            
            if (nodeIndexNW + width + 1 >= totalCells) return this;
            
            float heightNW = nodes[nodeIndexNW].height;
            float heightNE = nodes[nodeIndexNW + 1].height;
            float heightSW = nodes[nodeIndexNW + width].height;
            float heightSE = nodes[nodeIndexNW + width + 1].height;
            
            this.gradientX = (heightNE - heightNW) * (1.0f - y) + (heightSE - heightSW) * y;
            this.gradientY = (heightSW - heightNW) * (1.0f - x) + (heightSE - heightNE) * x;
            this.height = heightNW * (1.0f - x) * (1.0f - y) + heightNE * x * (1.0f - y) + heightSW * (1.0f - x) * y + heightSE * x * y;
            return this;
        }
        
        private void reset() {
            this.height = 0.0f;
            this.gradientX = 0.0f;
            this.gradientY = 0.0f;
        }
    }

    private static class Factory implements IntFunction<FreezeThaw> {
        private static final int SEED_OFFSET = 13578;
        private final int seed;
        private final Modifier modifier;
        private final FilterSettings.FreezeThaw settings;

        private Factory(final int seed, final FilterSettings filters, final Levels levels) {
            this.seed = seed + 13578;
            this.settings = filters.freezeThaw.copy();
            this.modifier = Modifier.range(levels.ground, levels.ground(15));
        }

        @Override
        public FreezeThaw apply(final int size) {
            return new FreezeThaw(this.seed, size, this.settings, this.modifier);
        }
    }
} 