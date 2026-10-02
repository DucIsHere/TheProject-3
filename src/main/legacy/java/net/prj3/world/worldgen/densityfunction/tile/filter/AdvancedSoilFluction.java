package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;
import java.util.Arrays;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.cell.heightmap.Levels;
import net.prj3.world.worldgen.noise.NoiseUtil;
import net.prj3.world.worldgen.util.FastRandom;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

public class AdvancedSoilFluction implements Filter {
    private final int mapSize;
    private final int iterations;
    private final FastRandom random;

    private final int iterations;

    private final float baseFlowRate;       // Tốc độ chảy cơ bản
    private final float maxSedimentFlow;    // Giới hạn khối lượng di chuyển/tick
    private final float weatheringRate;     // Tốc độ phong hóa đá thành đất bùn
    private final float lobeFriction;       // Lực cản khi tạo gờ bậc thang
    
    // --- MÔI TRƯỜNG ---
    private final float optimalFreezeThawTemp = 0.35f; // Điểm nhiệt độ tối ưu cho phong hóa nứt vỡ
    private final float snowLine;                      // Cao độ đường tuyết (thường là nơi lạnh nhất)
    private final Modifier modifier;                   // Vùng cho phép chạy Filter

    // Hằng số lưới Voxel
    private static final float ORTHO_DIST = 1.0f;
    private static final float DIAG_DIST = 1.4142135f;

    public AdvancedSoilFluction(int seed, int mapSize, int iterations, float baseFlowRate, float maxSedimentFlow, 
                                float weatheringRate, float lobeFriction, float snowLine, Modifier modifier) {
        this.mapSize = mapSize;
        this.iterations = iterations;
        this.random = new FastRandom(seed);
        this.baseFlowRate = baseFlowRate;
        this.maxSedimentFlow = maxSedimentFlow;
        this.weatheringRate = weatheringRate;
        this.lobeFriction = lobeFriction;
        this.snowLine = snowLine;
        this.modifier = modifier;
    }

    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int seedX, int seedZ, int iterationsPerChunk) {
        // Có thể dùng iterationsPerChunk hoặc this.iterations (ở đây ưu tiên config riêng của Solifluction)
        int actualIterations = Math.max(1, this.iterations);
        
        final Size size = map.getBlockSize();
        final int width = size.width();
        final int height = size.height();
        final int total = size.total();
        final Cell[] cells = map.getBacking();

        // Mảng Delta để lưu trữ sự thay đổi trầm tích, tránh Race Condition
        float[] deltaSediment = new float[total];

        for (int iter = 0; iter < actualIterations; iter++) {
            Arrays.fill(deltaSediment, 0.0f); // Reset buffer mỗi vòng lặp

            // --- BƯỚC 1: FROST WEATHERING (PHONG HÓA BĂNG GIÁ) ---
            for (int i = 0; i < total; i++) {
                Cell cell = cells[i];
                if (cell.erosionMask) continue;

                // Nhiệt độ càng gần điểm Freeze-Thaw, đá càng dễ nứt vỡ thành bùn
                float tempDiff = Math.abs(cell.regionTemperature - optimalFreezeThawTemp);
                float freezeThawFactor = Math.max(0.0f, 1.0f - (tempDiff / 0.15f)); 

                // Phong hóa mạnh hơn ở các sườn dốc (gradient cao) và độ cao gần snowline
                float heightFactor = Math.max(0.0f, 1.0f - Math.abs(cell.height - snowLine) * 2.0f);
                float weathering = weatheringRate * freezeThawFactor * heightFactor * cell.gradient;
                
                weathering = this.modifier.modify(cell, weathering);

                if (weathering > 0.001f) {
                    cell.height -= weathering;         // Bào mòn đá nền
                    cell.heightErosion -= weathering;  // Ghi nhận lịch sử bào mòn
                    cell.sediment += weathering;       // Sinh ra lớp đất mùn mới
                }
            }

            // --- BƯỚC 2: TÍNH TOÁN DÒNG CHẢY BÙN (SOLIFLUCTION FLOW) ---
            for (int z = 1; z < height - 1; z++) {
                for (int x = 1; x < width - 1; x++) {
                    int selfIdx = size.index(x, z);
                    Cell cell = cells[selfIdx];

                    if (cell.sediment <= 0.001f || cell.erosionMask) continue;

                    // Dynamic Yield Stress: Đất càng ẩm (Moisture cao) thì ngưỡng bắt đầu trượt càng thấp
                    float moistureFactor = NoiseUtil.clamp(cell.regionMoisture, 0.1f, 1.0f);
                    float localMinSlope = 0.05f * (1.0f - (moistureFactor * 0.8f));

                    float currentHeight = cell.height + cell.sediment;
                    float[] gradients = new float[8];
                    float totalDownhillGradient = 0.0f;

                    // Tính chênh lệch độ cao thực tế (bao gồm cả lớp bùn) ra 8 hướng
                    gradients[0] = calcGrad(currentHeight, cells[size.index(x, z - 1)], ORTHO_DIST);
                    gradients[1] = calcGrad(currentHeight, cells[size.index(x, z + 1)], ORTHO_DIST);
                    gradients[2] = calcGrad(currentHeight, cells[size.index(x + 1, z)], ORTHO_DIST);
                    gradients[3] = calcGrad(currentHeight, cells[size.index(x - 1, z)], ORTHO_DIST);
                    gradients[4] = calcGrad(currentHeight, cells[size.index(x + 1, z - 1)], DIAG_DIST);
                    gradients[5] = calcGrad(currentHeight, cells[size.index(x - 1, z - 1)], DIAG_DIST);
                    gradients[6] = calcGrad(currentHeight, cells[size.index(x + 1, z + 1)], DIAG_DIST);
                    gradients[7] = calcGrad(currentHeight, cells[size.index(x - 1, z + 1)], DIAG_DIST);

                    for (float g : gradients) {
                        if (g > localMinSlope) {
                            totalDownhillGradient += g;
                        }
                    }

                    // --- BƯỚC 3: PHÂN BỔ DÒNG CHẢY ---
                    if (totalDownhillGradient > 0) {
                        // Tốc độ chảy tỉ lệ thuận với độ dốc và độ ẩm
                        float flowCapacity = cell.sediment * this.baseFlowRate * moistureFactor * (totalDownhillGradient / 2.0f);
                        flowCapacity = Math.min(flowCapacity, this.maxSedimentFlow);
                        flowCapacity = this.modifier.modify(cell, flowCapacity);
                        
                        if (flowCapacity <= 0) continue;

                        deltaSediment[selfIdx] -= flowCapacity;

                        distribute(x, z - 1, gradients[0], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x, z + 1, gradients[1], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x + 1, z, gradients[2], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x - 1, z, gradients[3], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        
                        distribute(x + 1, z - 1, gradients[4], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x - 1, z - 1, gradients[5], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x + 1, z + 1, gradients[6], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                        distribute(x - 1, z + 1, gradients[7], localMinSlope, totalDownhillGradient, flowCapacity, size, deltaSediment);
                    }
                }
            }

            // --- BƯỚC 4: CẬP NHẬT & TẠO LOBES (THỀM BẬC THANG) ---
            for (int i = 0; i < total; i++) {
                if (deltaSediment[i] == 0) continue;

                Cell cell = cells[i];
                float delta = deltaSediment[i];

                // Solifluction Lobe effect: 
                // Khi đất bị đẩy vào một ô đang có độ dốc thấp (cell.gradient nhỏ), 
                // ma sát tăng vọt khiến đất ùn ứ lại tạo thành các gờ bậc thang.
                if (delta > 0 && cell.gradient < 0.2f) {
                    // Dồn cục bộ đất lại thay vì dàn phẳng
                    float lobeBuildUp = delta * this.lobeFriction;
                    cell.height += lobeBuildUp; 
                    cell.sediment += lobeBuildUp;
                } else {
                    cell.height += delta;
                    cell.sediment += delta;
                }

                // Chống sai số float
                if (cell.sediment < 0) cell.sediment = 0;
            }
        }
    }

    /**
     * Tính toán độ dốc giữa ô hiện tại và ô hàng xóm (có tính đến lớp trầm tích)
     */
    private float calcGrad(float myTotalHeight, Cell neighbor, float distance) {
        float neighborTotalHeight = neighbor.height + neighbor.sediment;
        return (myTotalHeight - neighborTotalHeight) / distance;
    }

    /**
     * Phân bổ đất trượt theo trọng số độ dốc
     */
    private void distribute(int nx, int nz, float gradient, float minSlope, float totalGradient, float totalFlow, Size size, float[] deltaBuffer) {
        if (gradient > minSlope) {
            float weight = gradient / totalGradient;
            float transfer = totalFlow * weight;
            deltaBuffer[size.index(nx, nz)] += transfer;
        }
    }

    /**
     * Factory tự động tiêm các thông số vật lý chuẩn mực.
     * Cực kỳ dễ dàng tích hợp vào WorldFilters.
     */
    public static IntFunction<AdvancedSoilFluction> factory(GeneratorContext context) {
        return (size) -> {
            Levels levels = context.levels;

            // Thông số chuẩn cho SoilFluction Đài nguyên (Tundra)
            int iterations = 4;
            float flowRate = 0.25f;        // Bùn lỏng trượt rất nhanh
            float maxSedimentFlow = 1.0f;  // Giới hạn max khối lượng mỗi tick
            float weatheringRate = 0.05f;  // Đá phân rã chậm
            float lobeFriction = 1.4f;     // Hệ số tạo gờ đất (lớn hơn 1 để dồn cục)
            
            // Đường băng giá, thông thường nằm ở 70% chiều cao thế giới
            float snowLine = levels.ground(100); 

            // Modifier: Tránh đất trượt phá hủy lòng sông sâu
            Modifier riverProtection = Modifier.range(levels.water - 5, levels.worldHeight).invert();

            return new AdvancedSoilFluction(
                context.seed.root() + 8472, 
                size, 
                s.iterations, 
                s.flowRate, 
                s.maxSedimentFlow, 
                s.weatheringRate, 
                s.lobeFriction, 
                s.snowLine, 
                s.riverProtection
            );
        };
    }
}