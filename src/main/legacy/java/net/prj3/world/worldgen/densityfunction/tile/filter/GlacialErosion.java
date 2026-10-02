package net.prj3.world.worldgen.densityfunction.tile.filter;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.heightmap.Levels;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.NoiseUtil;
import net.prj3.world.worldgen.util.FastRandom;

import java.util.Arrays;
import java.util.function.IntFunction;

/**
 * Advanced Glacial Erosion Filter
 * Kết hợp mô phỏng dòng chảy vật lý (Viscosity Flow) và bào mòn dựa trên hạt (Droplet Erosion)
 * Chuyên dụng để tạo địa hình thung lũng chữ U, vách đá mài mòn và gò đồi Moraine.
 */
public class GlacialErosion implements Filter {
    private final int seed;
    private final int mapSize;
    private final float snowLine;
    private final Modifier modifier;
    
    // Cài đặt vật lý chung
    private final float accumulationRate;
    private final float meltRate;
    private final float viscosity = 0.1F; // Độ nhớt của băng (càng cao băng càng dễ chảy)
    private final float pluckingProbability;
    private final float pluckingRate;

    // Cài đặt cho Hạt (Droplet)
    private final float erodeSpeed;
    private final float depositSpeed;
    private final float initialSpeed;
    private final float initialIceVolume;
    private final int maxIceLifetime;

    private final float iterations;
    
    // Cấu trúc Brush cho mài mòn chi tiết
    private final int[][] erosionBrushIndices;
    private final float[][] erosionBrushWeights;

    public GlacialErosion(int seed, int mapSize, float erodeSpeed, float depositSpeed, float snowLine, float iterations, Modifier modifier) {
        this.seed = seed;
        this.mapSize = mapSize;
        this.snowLine = snowLine;
        this.modifier = modifier;
        this.erodeSpeed = erodeSpeed;
        this.depositSpeed = depositSpeed;
        this.iterations = iterations;

        this.erosionBrushIndices = new int[mapSize * mapSize][];
        this.erosionBrushWeights = new float[mapSize * mapSize][];
        
        // Khởi tạo cọ bào mòn (Bán kính 6 để tạo thung lũng rộng)
        this.initBrushes(6); 
    }

    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int regionX, int regionZ, int iterations) {
        Size size = map.getBlockSize();
        Cell[] cells = map.getBacking();
        int total = size.total();

        // 1. Khởi tạo mảng mô phỏng dòng chảy (Cấp phát Local để tránh Race Condition)
        float[] iceMap = new float[total];
        float[] sedimentMap = new float[total];
        float[] fluxN = new float[total];
        float[] fluxS = new float[total];
        float[] fluxE = new float[total];
        float[] fluxW = new float[total];

        FastRandom random = new FastRandom(seed);

        // ==========================================
        // GIAI ĐOẠN 1: MÔ PHỎNG DÒNG CHẢY (GRID FLOW)
        // ==========================================
        int flowIterations = Math.max(1, iterations / 2); // Chạy một nửa số lần lặp cho dòng chảy
        for (int iter = 0; iter < flowIterations; iter++) {
            
            // BƯỚC 1: Tích tụ băng trên Snowline
            for (int i = 0; i < total; i++) {
                int x = i % size.width();
                int z = i / size.width();
                
                float noise = NoiseUtil.perlin2D(x, z, seed, 0.05f);
                float dynamicSnowLine = snowLine + (noise * 15.0f);

                if (cells[i].height > dynamicSnowLine) {
                    float altitudeFactor = (cells[i].height - dynamicSnowLine) * 2.0f;
                    iceMap[i] += accumulationRate * Math.max(1.0f, altitudeFactor);
                }
            }

            // BƯỚC 2: Tính toán dòng chảy dựa trên độ dốc và áp suất băng
            for (int x = 1; x < size.width() - 1; x++) {
                for (int z = 1; z < size.height() - 1; z++) {
                    int idx = size.index(x, z);
                    if (iceMap[idx] < 0.01f) continue;

                    float h_total = cells[idx].height + iceMap[idx];
                    
                    // Tính chênh lệch độ cao tổng thể (Địa hình + Băng)
                    float dN = h_total - (cells[size.index(x, z - 1)].height + iceMap[size.index(x, z - 1)]);
                    float dS = h_total - (cells[size.index(x, z + 1)].height + iceMap[size.index(x, z + 1)]);
                    float dE = h_total - (cells[size.index(x + 1, z)].height + iceMap[size.index(x + 1, z)]);
                    float dW = h_total - (cells[size.index(x - 1, z)].height + iceMap[size.index(x - 1, z)]);

                    float k = iceMap[idx] * viscosity;
                    fluxN[idx] = dN > 0 ? dN * k : 0;
                    fluxS[idx] = dS > 0 ? dS * k : 0;
                    fluxE[idx] = dE > 0 ? dE * k : 0;
                    fluxW[idx] = dW > 0 ? dW * k : 0;

                    float totalFlux = fluxN[idx] + fluxS[idx] + fluxE[idx] + fluxW[idx];
                    if (totalFlux > iceMap[idx]) {
                        float scale = iceMap[idx] / totalFlux;
                        fluxN[idx] *= scale; 
                        fluxS[idx] *= scale; 
                        fluxE[idx] *= scale; 
                        fluxW[idx] *= scale;
                    }
                }
            }

            // BƯỚC 3: Cập nhật bản đồ và thực hiện bào mòn sơ bộ
            for (int x = 1; x < size.width() - 1; x++) {
                for (int z = 1; z < size.height() - 1; z++) {
                    int idx = size.index(x, z);
                    
                    float iceIn = fluxS[size.index(x, z - 1)] + fluxN[size.index(x, z + 1)] +
                                  fluxW[size.index(x + 1, z)] + fluxE[size.index(x - 1, z)];
                    float iceOut = fluxN[idx] + fluxS[idx] + fluxE[idx] + fluxW[idx];

                    iceMap[idx] += iceIn - iceOut;

                    // Tan chảy (Ablation)
                    if (cells[idx].height < snowLine) {
                        float melt = (snowLine - cells[idx].height) * meltRate;
                        iceMap[idx] = Math.max(0, iceMap[idx] - melt);
                    }

                    // Bào mòn thung lũng diện rộng (Abrasion & Plucking)
                    if (iceMap[idx] > 0.5f && iceOut > 0.01f) {
                        float v = (float) Math.sqrt(iceOut);
                        float erosion = erodeSpeed * iceMap[idx] * v;
                        
                        float change = modifier.modify(cells[idx], erosion);
                        cells[idx].height -= change;
                        cells[idx].heightErosion -= change; // Lưu lại vết xói mòn
                        sedimentMap[idx] += change;

                        if (random.nextFloat() < pluckingProbability) {
                            float pluckChange = modifier.modify(cells[idx], pluckingRate);
                            cells[idx].height -= pluckChange;
                            cells[idx].heightErosion -= pluckChange;
                            sedimentMap[idx] += pluckChange;
                        }
                    }

                    // Advection: Di chuyển trầm tích theo dòng băng
                    if (iceOut > 0) {
                        float sedFraction = sedimentMap[idx] * (iceOut / (iceMap[idx] + 0.001f));
                        sedimentMap[idx] -= sedFraction;

                        float tFlux = fluxN[idx] + fluxS[idx] + fluxE[idx] + fluxW[idx];
                        if (tFlux > 0) {
                            sedimentMap[size.index(x, z - 1)] += sedFraction * (fluxN[idx] / tFlux);
                            sedimentMap[size.index(x, z + 1)] += sedFraction * (fluxS[idx] / tFlux);
                            sedimentMap[size.index(x + 1, z)] += sedFraction * (fluxE[idx] / tFlux);
                            sedimentMap[size.index(x - 1, z)] += sedFraction * (fluxW[idx] / tFlux);
                        }
                    }

                    // Nhả trầm tích khi băng tan hoàn toàn tạo Moraine
                    if (iceMap[idx] <= 0.05f && sedimentMap[idx] > 0) {
                        float depositAmount = modifier.modify(cells[idx], sedimentMap[idx]);
                        cells[idx].height += depositAmount;
                        cells[idx].sediment += depositAmount;
                        sedimentMap[idx] = 0;
                    }
                }
            }
        }

        // ==========================================
        // GIAI ĐOẠN 2: BÀO MÒN CHI TIẾT DỰA TRÊN HẠT (DROPLETS)
        // ==========================================
        TerrainPos grad1 = new TerrainPos();
        TerrainPos grad2 = new TerrainPos();
        
        int lengthChunks = size.total() >> 4;
        for (int i = 0; i < iterations; ++i) {
            final long iterationSeed = NoiseUtil.seed(this.seed, i);
            for (int cx = 0; cx < lengthChunks; ++cx) {
                for (int cz = 0; cz < lengthChunks; ++cz) {
                    random.seed(NoiseUtil.seed(cx, cz), iterationSeed);
                    
                    float px = (float) ((cx << 4) + random.nextInt(16));
                    float pz = (float) ((cz << 4) + random.nextInt(16));
                    
                    px = NoiseUtil.clamp(px, 1.0f, size.width() - 2);
                    pz = NoiseUtil.clamp(pz, 1.0f, size.height() - 2);
                    
                    if (cells[size.index((int)px, (int)pz)].height > snowLine) {
                        applyGlacierParticle(px, pz, cells, size, grad1, grad2);
                    }
                }
            }
        }
    }

    private void applyGlacierParticle(float posX, float posZ, Cell[] cells, Size size, TerrainPos grad1, TerrainPos grad2) {
        float dirX = 0.0f, dirZ = 0.0f;
        float sediment = 0.0f;
        float speed = initialSpeed;
        float ice = initialIceVolume;

        grad1.reset(); grad2.reset();

        for (int life = 0; life < maxIceLifetime; life++) {
            int nodeX = (int) posX;
            int nodeZ = (int) posZ;
            int idx = size.index(nodeX, nodeZ);
            float offX = posX - nodeX;
            float offZ = posZ - nodeZ;

            grad1.compute(cells, size, posX, posZ);
            
            // Quán tính của khối băng rất lớn, chuyển hướng chậm
            dirX = dirX * 0.25f - grad1.gx * 0.75f;
            dirZ = dirZ * 0.25f - grad1.gz * 0.75f;

            float len = (float) Math.sqrt(dirX * dirX + dirZ * dirZ);
            if (len > 0) { dirX /= len; dirZ /= len; }

            posX += dirX; posZ += dirZ;

            if (posX < 1 || posX >= size.width() - 2 || posZ < 1 || posZ >= size.height() - 2) break;

            float newH = grad2.compute(cells, size, posX, posZ).h;
            float deltaH = newH - grad1.h;

            // Sức tải trầm tích của băng hà tỷ lệ thuận với độ dày băng và vận tốc
            float capacity = Math.max(-deltaH * speed * ice * 8.0f, 0.05f);

            if (cells[idx].height < snowLine) ice *= 0.85f; // Tan chảy nhanh
            else ice *= 0.999f; // Thăng hoa ở vùng cao

            if (sediment > capacity || deltaH > 0 || ice < 0.01f) {
                // Trầm tích (Moraines)
                float deposit = (deltaH > 0) ? Math.min(deltaH, sediment) : (sediment - capacity) * depositSpeed;
                sediment -= deposit;

                addDeposit(cells, size, nodeX, nodeZ, deposit, offX, offZ);
                if (ice < 0.01f) break;
            } else {
                // Mài mòn diện rộng (U-Shape Valley) thông qua Brush
                float erode = Math.min((capacity - sediment) * erodeSpeed, -deltaH);
                for (int b = 0; b < erosionBrushIndices[idx].length; b++) {
                    int bIdx = erosionBrushIndices[idx][b];
                    float w = erosionBrushWeights[idx][b];
                    float actualErode = modifier.modify(cells[bIdx], erode * w);
                    
                    cells[bIdx].height -= actualErode;
                    cells[bIdx].heightErosion -= actualErode;
                    sediment += actualErode;
                }
            }

            speed = (float) Math.sqrt(speed * speed + deltaH * 1.5f);
            if (Float.isNaN(speed)) speed = 0.0f;
        }
    }

    private void addDeposit(Cell[] cells, Size size, int x, int z, float amt, float ox, float oz) {
        int idx = size.index(x, z);
        float wNW = (1 - ox) * (1 - oz);
        float wNE = ox * (1 - oz);
        float wSW = (1 - ox) * oz;
        float wSE = ox * oz;

        depositCell(cells[idx], amt * wNW);
        depositCell(cells[idx + 1], amt * wNE);
        depositCell(cells[idx + size.width()], amt * wSW);
        depositCell(cells[idx + size.width() + 1], amt * wSE);
    }
    
    private void depositCell(Cell cell, float amt) {
        if (!cell.erosionMask) {
            float change = modifier.modify(cell, amt);
            cell.height += change;
            cell.sediment += change;
        }
    }

    private void initBrushes(int radius) {
        for (int i = 0; i < erosionBrushIndices.length; i++) {
            int cx = i % mapSize;
            int cz = i / mapSize;

            int[] tempIndices = new int[radius * radius * 4];
            float[] tempWeights = new float[radius * radius * 4];
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
                            float w = 1.0f - (d * d); // Falloff bậc 2 tối ưu cho thung lũng chữ U
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
            for (int j = 0; j < countp; j++) erosionBrushWeights[i][j] = tempWeights[j] / sum;
        }
    }

    private static class TerrainPos {
        float h, gx, gz;
        
        TerrainPos compute(Cell[] cells, Size size, float x, float z) {
            int ix = (int) x; int iz = (int) z;
            float fx = x - ix; float fz = z - iz;
            int idx = size.index(ix, iz);
            
            float hNW = cells[idx].height;
            float hNE = cells[idx + 1].height;
            float hSW = cells[idx + size.width()].height;
            float hSE = cells[idx + size.width() + 1].height;
            
            gx = (hNE - hNW) * (1 - fz) + (hSE - hSW) * fz;
            gz = (hSW - hNW) * (1 - fx) + (hSE - hNE) * fx;
            h = hNW * (1 - fx) * (1 - fz) + hNE * fx * (1 - fz) + hSW * (1 - fx) * fz + hSE * fx * fz;
            return this;
        }
        
        void reset() { h = 0; gx = 0; gz = 0; }
    }

    public static IntFunction<GlacialErosionFull> factory(GeneratorContext context) {
        return (size) -> {
            Levels levels = context.levels;
            Modifier mod = Modifier.range(levels.ground, levels.ground(200)).invert();
            // Khởi tạo factory cứng với thông số chuẩn, snowline lấy từ mực nước + x
            float snowLine = levels.ground(90); 
            return new GlacialErosionFull(context.seed.root() + 14567, size, 0.15f, 0.20f, snowLine, mod);
        };
    }
}