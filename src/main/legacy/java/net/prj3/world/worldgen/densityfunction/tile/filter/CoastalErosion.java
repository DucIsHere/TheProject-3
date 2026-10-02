package net.prj3.world.worldgen.densityfunction.tile.filter;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

import java.util.Arrays;
import java.util.function.IntFunction;

/**
 * Advanced CoastalErosion Filter
 * Phiên bản nâng cấp độ chi tiết cực cao:
 * - Time-varying boundary: Mô phỏng mực nước biển thay đổi theo thời gian (Thủy triều).
 * - Exact Normal Vector: Tính toán Pháp tuyến bề mặt với độ chính xác cao để lấy góc va đập sóng.
 * - Wave Dynamics: Lực sóng đánh bào mòn vách đá và tạo hàm ếch (Notch Formation).
 * - Mass Wasting: Đánh giá sụp đổ mảng lớn khi chân vách bị khoét sâu.
 * - Sediment Transport: Trầm tích sinh ra từ xói mòn trôi dạt xuống vùng nước nông.
 */
public class CoastalErosion implements Filter {
    private final int mapSize;
    private final float baseWaterLevel;
    
    // Thông số động lực học chất lỏng và vật lý
    private final Noise waveDirectionNoise; // Noise quy định hướng đi của sóng
    private final float erosionScale;       // Độ mạnh tổng thể của lực sóng
    private final float abrasionFactor;     // Tỷ lệ vỡ của đá thành trầm tích (cát/sỏi)
    private final float tideAmplitude;      // Biên độ thủy triều (bao nhiêu block)
    private final float tideFrequency;      // Chu kỳ thủy triều
    private final float criticalAngle;      // Góc sạt lở tới hạn (Repose Angle)
    private final float iterations;
    private final Modifier modifier;

    public CoastalErosion(int mapSize, float waterLevel, Noise waveNoise, float erosionScale, float abrasionFactor, float iterations, Modifier modifier) {
        this.mapSize = mapSize;
        this.baseWaterLevel = waterLevel;
        this.waveDirectionNoise = waveNoise;
        this.erosionScale = erosionScale;
        this.abrasionFactor = abrasionFactor;
        this.iterations = iterations;
        this.modifier = modifier;
        
        // Cấu hình Thủy triều và Địa chất
        this.tideAmplitude = 2.5f;   // Nước lên xuống 2.5 blocks
        this.tideFrequency = 0.4f;   // Tần số thay đổi qua các iterations
        this.criticalAngle = 2.0f;   // Chênh lệch độ cao > 2.0 sẽ gây sạt lở
    }

    @Override
    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int seedX, int seedZ, int iterations) {
        final Size size = map.getBlockSize();
        // Dùng size.total() do kiến trúc grid đồng nhất (width = height = total)
        final int total = size.total(); 
        
        // Mảng Double Buffering để cập nhật đồng bộ sạt lở và trầm tích
        float[] collapseBuffer = new float[total * total];
        float[] sedimentBuffer = new float[total * total];

        for (int iter = 0; iter < iterations; iter++) {
            Arrays.fill(collapseBuffer, 0.0f);
            Arrays.fill(sedimentBuffer, 0.0f);
            
            // 1. TIME-VARYING BOUNDARY (Mô phỏng Thủy triều)
            // Mực nước dao động theo hàm Sine dựa trên số bước thời gian (iterations)
            float currentWaterLevel = this.baseWaterLevel + (float) Math.sin(iter * this.tideFrequency) * this.tideAmplitude;

            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int idx = z * total + x;
                    Cell cell = map.getCellRaw(x, z);

                    // Bỏ qua các ô nước quá sâu hoặc các vùng núi cao tít không thể bị sóng chạm tới
                    if (cell.height < currentWaterLevel - 5.0f || cell.height > currentWaterLevel + 15.0f) continue;
                    
                    // 2. NORMAL VECTOR CALCULATION (Phương pháp Sai phân trung tâm - Central Difference)
                    // dz/dx = (h(x+1, z) - h(x-1, z)) / 2
                    float dzdx = (map.getCellRaw(x + 1, z).height - map.getCellRaw(x - 1, z).height) * 0.5f;
                    // dz/dz = (h(x, z+1) - h(x, z-1)) / 2
                    float dzdz = (map.getCellRaw(x, z + 1).height - map.getCellRaw(x, z - 1).height) * 0.5f;
                    
                    // Vector pháp tuyến N = (-dz/dx, 1.0, -dz/dz)
                    float length = (float) Math.sqrt(dzdx * dzdx + 1.0f + dzdz * dzdz);
                    float nx = -dzdx / length;
                    float ny = 1.0f / length;
                    float nz = -dzdz / length;

                    // Tính độ dốc (Slope) dựa trên thành phần Y của pháp tuyến
                    float slope = 1.0f - ny;

                    // 3. TÍNH TOÁN LỰC SÓNG VÀ GÓC VA ĐẬP (Wave Dynamics & Dot Product)
                    // Lấy góc sóng từ miền Noise [0, 1] ánh xạ ra radian [0, 2π]
                    float waveAngle = (float) this.waveDirectionNoise.compute(map.getBlockX() + x, map.getBlockZ() + z, seedX) * (float) Math.PI * 2.0f;
                    float waveDirX = (float) Math.cos(waveAngle);
                    float waveDirZ = (float) Math.sin(waveAngle);
                    
                    // Đánh giá mức độ tiếp xúc với vùng biển mở (Exposure)
                    float exposure = calculateExposure(map, x, z, currentWaterLevel);
                    if (exposure <= 0) continue;

                    // Tích vô hướng (Dot Product) giữa Hướng Sóng (đảo ngược vì sóng đập vào bờ) và Pháp tuyến mặt phẳng
                    // Nếu Dot > 0: Mặt đón sóng (Đón lực lớn nhất)
                    // Nếu Dot <= 0: Khuất sóng (Bờ leeward)
                    float dotProduct = -(waveDirX * nx + waveDirZ * nz);
                    
                    if (dotProduct > 0.1f && slope > 0.2f) { // Chỉ xói mòn nếu dốc đủ đứng và đón sóng
                        
                        // Năng lượng sóng = Góc đón sóng * Độ mở của biển * Sức mạnh cơ bản
                        float wavePower = dotProduct * exposure * this.erosionScale;

                        // 4. NOTCH FORMATION (Tạo ngàm ếch)
                        // Bào mòn mạnh nhất ở vùng giao cắt mực nước (Swash zone)
                        float heightDiff = cell.height - currentWaterLevel;
                        // Phân bố lực bào mòn dạng hình chuông (Gaussian curve) bám theo mực nước
                        float notchFactor = (float) Math.exp(-0.4 * (heightDiff * heightDiff)); 
                        
                        float erosionAmount = wavePower * notchFactor;
                        erosionAmount = this.modifier.modify(cell, erosionAmount);

                        if (erosionAmount > 0.001f) {
                            // Gọt vách đá
                            cell.height -= erosionAmount;
                            cell.heightErosion += erosionAmount;
                            
                            // Sinh trầm tích (đá vỡ ra thành cát/sỏi)
                            float generatedSediment = erosionAmount * this.abrasionFactor;
                            sedimentBuffer[idx] += generatedSediment;

                            // 5. MÔ PHỎNG MASS COLLAPSE (Kiểm tra sạt lở)
                            // Nếu sóng khoét sâu tạo hàm ếch quá lớn, phần vách bên trên sẽ mất trọng tâm và sập xuống
                            checkCliffCollapse(map, x, z, erosionAmount, collapseBuffer, total, nx, nz);
                        }
                    }
                }
            }

            // 6. GIẢI QUYẾT DOUBLE BUFFERING & VẬN CHUYỂN TRẦM TÍCH
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    int i = z * total + x;
                    Cell c = map.getCellRaw(x, z);
                    
                    // Xử lý sập vách đá
                    if (collapseBuffer[i] > 0) {
                        c.height -= collapseBuffer[i];
                        c.heightErosion += collapseBuffer[i];
                        
                        // Khối sập cũng sinh ra trầm tích cực lớn trượt xuống dưới
                        sedimentBuffer[i] += collapseBuffer[i] * 0.9f; 
                    }
                    
                    // Rửa trôi trầm tích (Sediment Transport)
                    if (sedimentBuffer[i] > 0) {
                        transportSediment(map, x, z, sedimentBuffer[i], currentWaterLevel);
                    }
                }
            }
        }
    }

    /**
     * Tính toán mức độ tiếp xúc với biển (Exposure).
     * Phóng Ray-marching cự ly ngắn để xem trước mặt có phải là biển mở hay bị chắn.
     */
    private float calculateExposure(Filterable map, int x, int z, float waterLevel) {
        int waterNeighbors = 0;
        float depthSum = 0;
        
        for (int dz = -1; dz <= 1; dz++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dz == 0) continue;
                Cell n = map.getCellRaw(x + dx, z + dz);
                if (n.height < waterLevel) {
                    waterNeighbors++;
                    depthSum += (waterLevel - n.height);
                }
            }
        }
        
        if (waterNeighbors == 0) return 0;
        // Độ sâu càng lớn -> sóng càng mạnh (sóng ít bị tiêu hao năng lượng do ma sát đáy)
        float avgDepth = depthSum / waterNeighbors;
        return (waterNeighbors / 8.0f) * Math.min(1.0f, avgDepth / 8.0f);
    }

    /**
     * Thuật toán kéo phần đất nứt gãy ở phía trên sụp xuống (Mass Wasting)
     * Dựa trên Vector pháp tuyến ngang (nx, nz) để tìm đúng hướng của sườn dốc (đỉnh dốc nằm ngược chiều pháp tuyến).
     */
    private void checkCliffCollapse(Filterable map, int x, int z, float erosion, float[] buffer, int total, float nx, float nz) {
        // Hướng "lên bờ" (Inland) ngược chiều pháp tuyến
        int dirX = (int) Math.signum(-nx);
        int dirZ = (int) Math.signum(-nz);
        
        if (dirX == 0 && dirZ == 0) return;

        int inlandX = x + dirX;
        int inlandZ = z + dirZ;

        Cell current = map.getCellRaw(x, z);
        Cell inland = map.getCellRaw(inlandX, inlandZ);
        
        // Tính độ dốc cục bộ
        float slopeDelta = inland.height - current.height;
        
        // Nếu độ dốc vượt quá góc nghỉ an toàn (Repose Angle / Critical Angle) -> Bắt đầu sạt lở
        if (slopeDelta > this.criticalAngle) {
            float collapseAmount = (slopeDelta - this.criticalAngle) * 0.45f;
            buffer[inlandZ * total + inlandX] += collapseAmount;
            
            // Lan truyền ứng suất đứt gãy lên ô cao hơn nữa (Chain-reaction collapse)
            if (collapseAmount > 1.5f) {
                int deepInlandX = inlandX + dirX;
                int deepInlandZ = inlandZ + dirZ;
                Cell deepInland = map.getCellRaw(deepInlandX, deepInlandZ);
                if (deepInland.height > inland.height) {
                    buffer[deepInlandZ * total + deepInlandX] += collapseAmount * 0.35f;
                }
            }
        }
    }

    /**
     * Rửa trôi trầm tích xuống dốc (Theo mô hình Cellular Automata).
     * Trầm tích sẽ lăn tới vùng nước yên tĩnh hơn và tạo thành các bãi bồi nông (Shoals/Sandbars).
     */
    private void transportSediment(Filterable map, int x, int z, float amount, float waterLevel) {
        Cell cell = map.getCellRaw(x, z);
        float minHeight = cell.height;
        int targetX = x;
        int targetZ = z;

        // Tìm điểm thấp nhất xung quanh bằng cách quét 8 hướng
        int[] dx = {0, 0, 1, -1, 1, -1, 1, -1};
        int[] dz = {1, -1, 0, 0, 1, 1, -1, -1};

        for (int i = 0; i < 8; i++) {
            Cell n = map.getCellRaw(x + dx[i], z + dz[i]);
            if (n.height < minHeight) {
                minHeight = n.height;
                targetX = x + dx[i];
                targetZ = z + dz[i];
            }
        }

        // Nếu có chỗ thấp hơn, di chuyển trầm tích
        if (targetX != x || targetZ != z) {
            Cell target = map.getCellRaw(targetX, targetZ);
            
            // Nếu điểm đến dưới mực nước -> Lắng đọng bồi đắp đáy biển
            if (target.height < waterLevel) {
                target.height += amount * 0.6f;
                target.sediment += amount * 0.4f; // Vẫn còn bùn/cát mịn tiếp tục trôi lơ lửng
            } else {
                target.sediment += amount; // Ở trên cạn, tiếp tục trượt xuống ở frame tiếp theo
            }
        } else {
            // Lắng đọng tại chỗ
            cell.sediment += amount;
            if (cell.height < waterLevel) {
                cell.height += amount * 0.3f;
            }
        }
    }

    public static IntFunction<CoastalErosion> factory(GeneratorContext context) {
        return (size) -> new CoastalErosion(
            size,
            context.levels.water,
            context.windX, // Dùng noise gió toàn cục của engine để tạo hướng sóng nhất quán!
            0.75f,         // Lực xói mòn được scale
            0.65f,         // 65% mảnh vỡ rơi ra thành cát bùn
            Modifier.range(context.levels.water - 8, context.levels.water + 35)
        );
    }
}