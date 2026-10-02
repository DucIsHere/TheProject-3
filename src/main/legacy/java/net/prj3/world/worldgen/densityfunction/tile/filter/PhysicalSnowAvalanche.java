package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

/**
 * PhysicalSnowAvalanche: Mô phỏng lở tuyết dựa trên động lực học chất lỏng phi Newton.
 * Áp dụng mô hình Voellmy-Salm và phân tích ổn định mái dốc.
 */
public class PhysicalSnowAvalanche implements Filter {
    // --- HẰNG SỐ VẬT LÝ ĐỊA CHẤT ---
    private static final float Gravity = 9.806f;            // Gia tốc trọng trường (m/s^2)
    private static final float RHO_SNOW = 300.0f;     // Khối lượng riêng tuyết (kg/m^3)
    private static final float MU = 0.25f;            // Hệ số ma sát Coulomb (tuyết-đá)
    private static final float XI = 1500.0f;          // Hệ số ma sát nhớt (Turbulent friction)
    private static final float TENSILE_STRENGTH = 500.0f; // Sức bền kéo của lớp tuyết (Pa)

    private final int mapSize;
    private final float minSnowHeight;
    private final float iterations;
    private final Noise snowNoise;
    private final Modifier modifier;

    public PhysicalSnowAvalanche(int mapSize, float minSnowHeight, float iterations, Noise snowNoise, Modifier modifier) {
        this.mapSize = mapSize;
        this.minSnowHeight = minSnowHeight;
        this.snowNoise = snowNoise;
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
        
        // Momentum Buffer (Lưu trữ động lượng để tính quán tính trượt)
        final float[] velocityMap = new float[total];
        final int[] snowDepth = new float[total];

        for (int iter = 0; iter < iterations; iter++) {
            // PHA 1: PHÂN TÍCH ỨNG SUẤT (STRESS ANALYSIS)
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int idx = z * total + x;
                    Cell cell = map.getCellRaw(x, z);
                    if (cell.erosionMask || cell.height < minSnowHeight) continue;

                    // Tính độ dày tuyết thực tế dựa trên địa hình (Concavity/Convexity)
                    float thickness = (float) Math.max(0, snowNoise.compute(worldX + x, worldZ + z, seedX) * 5.0f);
                    
                    // Tính Gradient & Góc dốc (Theta)
                    float dzdx = (map.getCellRaw(x + 1, z).height - map.getCellRaw(x - 1, z).height) / 2.0f;
                    float dzdy = (map.getCellRaw(x, z + 1).height - map.getCellRaw(x, z - 1).height) / 2.0f;
                    float slope = (float) Math.sqrt(dzdx * dzdx + dzdy * dzdy);
                    float theta = (float) Math.atan(slope);

                    // Phương trình Ứng suất cắt: Tau = Rho * g * h * sin(theta)
                    float shearStress = RHO_SNOW * G * thickness * (float) Math.sin(theta);
                    
                    // Hệ số an toàn (Factor of Safety)
                    // FS = (Resisting Strength) / (Driving Stress)
                    float resistingStrength = TENSILE_STRENGTH + (RHO_SNOW * G * thickness * (float) Math.cos(theta) * MU);
                    float FS = resistingStrength / (shearStress + 0.1f);

                    if (FS < 1.2f) { // Nếu vùng này không ổn định
                        snowDepth[idx] = thickness;
                    }
                }
            }

            // PHA 2: DÒNG CHẢY VOELLMY (PHYSICS FLOW)
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int idx = z * total + x;
                    if (snowDepth[idx] <= 0) continue;

                    // Tìm hướng chảy dốc nhất
                    int bestX = x, bestZ = z;
                    float minNeighborHeight = map.getCellRaw(x, z).height;
                    
                    for (int dz = -1; dz <= 1; dz++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            float h = map.getCellRaw(x + dx, z + dz).height;
                            if (h < minNeighborHeight) {
                                minNeighborHeight = h;
                                bestX = x + dx; bestZ = z + dz;
                            }
                        }
                    }

                    if (bestX == x && bestZ == z) continue; // Điểm tụ

                    // Áp dụng phương trình gia tốc Voellmy
                    float dz = map.getCellRaw(x, z).height - minNeighborHeight;
                    float theta = (float) Math.atan(dz);
                    float v = velocityMap[idx];
                    float h = snowDepth[idx];

                    // a = g(sinθ - μcosθ) - (g * v^2) / (h * ξ)
                    float acceleration = G * ((float)Math.sin(theta) - MU * (float)Math.cos(theta)) 
                                       - (G * v * v) / (Math.max(0.1f, h) * XI);
                    
                    float vNew = Math.max(0, v + acceleration);
                    float flowAmount = h * (vNew / (vNew + 1.0f)) * 0.5f; // Lưu lượng trượt

                    // Thực thi dịch chuyển khối lượng
                    map.getCellRaw(x, z).height -= flowAmount;
                    map.getCellRaw(bestX, bestZ).height += flowAmount;
                    
                    // Bào mòn bề mặt do áp suất động (Dynamic Erosion)
                    float dynamicPressure = 0.5f * RHO_SNOW * vNew * vNew;
                    float groundErosion = (dynamicPressure / 100000.0f) * modifier.modify(map.getCellRaw(x, z), 1.0f);
                    map.getCellRaw(x, z).height -= groundErosion;
                    map.getCellRaw(x, z).heightErosion += groundErosion;

                    // Truyền động năng cho ô tiếp theo
                    velocityMap[bestZ * total + bestX] = vNew * 0.9f; 
                    snowDepth[idx] -= flowAmount;
                    snowDepth[bestZ * total + bestX] += flowAmount;
                }
            }
        }
    }

    public static IntFunction<PhysicalSnowAvalanche> factory(GeneratorContext context) {
        return (size) -> new PhysicalSnowAvalanche(size, 100.0f, context.stabilityNoise, Modifier.DEFAULT);
    }
}