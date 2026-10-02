package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;
import java.util.Arrays;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.cell.heightmap.Levels;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.noise.module.Noises;

public class AeroErosion implements Filter {
    private final int mapSize;
    private final Noise windX;
    private final Noise windZ;
    private final float erodeStrength;
    private final float depositStrength;
    private final float minHeight;
    private final float iterations
    private final Modifier modifier;

    public AeroErosion(int mapSize, Noise windX, Noise windZ, float erodeStrength, float depositStrength, float minHeight, float iterations, Modifier modifier) {
        this.mapSize = mapSize;
        this.windX = windX;
        this.windZ = windZ;
        this.erodeStrength = erodeStrength;
        this.depositStrength = depositStrength;
        this.minHeight = minHeight;
        this.iterations = iterations;
        this.modifier = modifier;
    }

    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int seedX, int seedZ, int iterations) {
        final Size size = map.getBlockSize();
        final int total = size.total();
        final int worldX = map.getBlockX();
        final int worldZ = map.getBlockZ();

        // Mảng đệm lưu trữ lượng bụi (sediment) đang bay trong không khí tại mỗi ô
        float[] airDust = new float[total * total];

        for (int iter = 0; iter < iterations; iter++) {
            
            // BƯỚC 1: BÀO MÒN VÀ BỐC BỤI (Abrasion & Suspension)
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int index = z * total + x;
                    Cell cell = map.getCellRaw(x, z);

                    if (cell.height < this.minHeight || cell.terrain.isSubmerged() || cell.erosionMask) continue;

                    // Tính toán Vector Gió
                    float wx = this.windX.compute(worldX + x, worldZ + z, seedX);
                    float wz = this.windZ.compute(worldX + x, worldZ + z, seedZ);
                    float wLen = (float) Math.sqrt(wx * wx + wz * wz);
                    if (wLen < 0.0001f) continue;
                    wx /= wLen; wz /= wLen;

                    // Tính Pháp tuyến bề mặt (Central Difference)
                    float dx = (map.getCellRaw(x + 1, z).height - map.getCellRaw(x - 1, z).height) * 0.5f;
                    float dz = (map.getCellRaw(x, z + 1).height - map.getCellRaw(x, z - 1).height) * 0.5f;
                    
                    float nLen = (float) Math.sqrt(dx * dx + 1.0f + dz * dz);
                    float nx = -dx / nLen;
                    float nz = -dz / nLen;

                    // Dot Product: Phơi nhiễm gió
                    float exposure = (wx * nx) + (wz * nz);

                    if (exposure > 0.05f) {
                        // Gió đập vào mặt đá: Bào mòn bề mặt
                        float eroded = this.modifier.modify(cell, exposure * this.erodeStrength);
                        cell.height -= eroded;
                        cell.heightErosion += eroded;
                        
                        // Đất bị gọt biến thành bụi bay trong không khí
                        airDust[index] += eroded; 
                    }
                    
                    // Gió cuốn bụi mịn có sẵn trên mặt đất (Deflation)
                    if (cell.sediment > 0) {
                        float lift = cell.sediment * 0.2f * (1.0f + exposure);
                        cell.sediment -= lift;
                        airDust[index] += lift;
                    }
                }
            }

            // BƯỚC 2: VẬN CHUYỂN VÀ LẮNG ĐỌNG (Advection & Deposition)
            float[] nextAirDust = new float[total * total];
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int index = z * total + x;
                    float dust = airDust[index];
                    if (dust <= 0) continue;

                    Cell cell = map.getCellRaw(x, z);
                    
                    // Tính hướng gió để biết bụi bay đi đâu
                    float wx = this.windX.compute(worldX + x, worldZ + z, seedX);
                    float wz = this.windZ.compute(worldX + x, worldZ + z, seedZ);
                    float wLen = (float) Math.sqrt(wx * wx + wz * wz);
                    if (wLen > 0.0001f) { wx /= wLen; wz /= wLen; }

                    // Tính độ dốc cục bộ: Bụi không lắng đọng được trên vách đứng
                    float slope = (float) Math.sqrt(
                        Math.pow(map.getCellRaw(x + 1, z).height - cell.height, 2) + 
                        Math.pow(map.getCellRaw(x, z + 1).height - cell.height, 2)
                    );

                    // Lắng đọng: Ưu tiên vùng dốc thấp (thung lũng) và khuất gió
                    float depositChance = this.depositStrength / (1.0f + slope * 5.0f);
                    float deposited = dust * depositChance;
                    
                    cell.height += deposited;
                    cell.sediment += deposited;
                    dust -= deposited;

                    // Phần bụi còn lại bay sang ô kế tiếp theo hướng gió
                    if (dust > 0) {
                        int tx = x + Math.round(wx);
                        int tz = z + Math.round(wz);
                        if (tx >= 0 && tx < total && tz >= 0 && tz < total) {
                            nextAirDust[tz * total + tx] += dust;
                        }
                    }
                }
            }
            airDust = nextAirDust; // Cập nhật bụi cho lần lặp kế tiếp
        }
    }

    public static IntFunction<AeroErosion> factory(GeneratorContext context) {
        return (size) -> {
            Modifier mod = Modifier.range(context.levels.ground, context.levels.ground(120)).invert();
            return new AeroErosion(size, context.windX, context.windZ, 0.015F, 0.3F, context.levels.water + 1.0F, mod);
        };
    }
}