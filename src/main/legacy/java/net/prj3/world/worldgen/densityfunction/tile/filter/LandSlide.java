package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

/**
 * LandSlide: Mô phỏng sạt lở khối và tạo hố sụt (Sinkholes) do đứt gãy địa chất.
 * Được thiết kế để chạy sau ForceErosion và trước ThermalErosion.
 */
public class LandSlide implements Filter {
    private final int mapSize;
    private final Noise stabilityNoise; 
    private final float collapseThreshold;
    private final float iterations;
    private final Modifier modifier;

    public LandSlide(int mapSize, Noise stabilityNoise, float collapseThreshold, float iterations, Modifier modifier) {
        this.mapSize = mapSize;
        this.stabilityNoise = stabilityNoise;
        this.collapseThreshold = collapseThreshold;
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

        for (int iter = 0; iter < iterations; iter++) {
            for (int z = 1; z < total - 1; z++) {
                for (int x = 1; x < total - 1; x++) {
                    Cell cell = map.getCellRaw(x, z);

                    // Bỏ qua nếu khu vực đã bị khóa xói mòn (ví dụ: đáy sông, hẻm núi Faulting)
                    if (cell.erosionMask) {
                        continue;
                    }

                    float stability = stabilityNoise.compute(worldX + x, worldZ + z, seedX);

                    // Nếu địa chất yếu (stability thấp hơn ngưỡng sụp đổ)
                    if (stability < this.collapseThreshold) {
                        Cell n = map.getCellRaw(x, z - 1);
                        Cell s = map.getCellRaw(x, z + 1);
                        Cell e = map.getCellRaw(x + 1, z);
                        Cell w = map.getCellRaw(x - 1, z);

                        float avgNeighborHeight = (n.height + s.height + e.height + w.height) * 0.25f;
                        float diff = cell.height - avgNeighborHeight;

                        // Cường độ sụp đổ tỷ lệ nghịch với độ ổn định
                        float intensity = 1.0f - (stability / this.collapseThreshold);

                        // 1. LANDSLIDE: Khối trượt (Vách núi sập) do ForceErosion đẩy lên quá cao
                        if (diff > 1.5f) {
                            float slideAmount = diff * 0.35f * intensity;
                            slideAmount = this.modifier.modify(cell, slideAmount);

                            // Gọt cấu trúc đá cứng
                            cell.height -= slideAmount;
                            cell.heightErosion += slideAmount; // Ghi nhận để map xuất khối đá lởm chởm

                            // Lượng đá sập nát ra thành trầm tích (sediment) để Gió/Nước cuốn đi
                            float sedimentGenerated = slideAmount * 0.8f; 
                            cell.sediment += slideAmount * 0.2f; // Giữ lại một ít tại chỗ

                            // Đá vụn đổ dồn xuống các ô lân cận thấp hơn
                            distributeToLowest(centerHeight, sedimentGenerated, n, s, e, w);
                        } 
                        // 2. SINKHOLE: Hố sụt (Đáy sập xuống) do địa chất bở rời
                        else if (diff < -1.5f) {
                            float sinkAmount = Math.abs(diff) * 0.4f * intensity;
                            sinkAmount = this.modifier.modify(cell, sinkAmount);

                            // Sụp sâu xuống
                            cell.height -= sinkAmount;
                            cell.heightErosion += sinkAmount;

                            // Kéo đất từ các ô cao xung quanh sụt theo miệng hố
                            pullNeighborsDown(cell.height, sinkAmount * 0.5f, n, s, e, w);
                        }
                    }
                }
            }
        }
    }

    /**
     * Sạt lở: Trầm tích chỉ tràn xuống các khu vực lân cận thấp hơn ô trung tâm.
     */
    private void distributeToLowest(float centerHeight, float amount, Cell... neighbors) {
        int validCount = 0;
        for (Cell neighbor : neighbors) {
            if (!neighbor.erosionMask && neighbor.height < centerHeight) {
                validCount++;
            }
        }

        if (validCount > 0) {
            float amountPerCell = amount / validCount;
            for (Cell neighbor : neighbors) {
                if (!neighbor.erosionMask && neighbor.height < centerHeight) {
                    // Đá tảng lăn xuống đập nát thêm cấu trúc bên dưới, chuyển thành trầm tích
                    neighbor.sediment += amountPerCell; 
                }
            }
        }
    }

    /**
     * Hố sụt: Kéo đất từ các ô cao hơn tụt xuống đáy hố.
     */
    private void pullNeighborsDown(float centerHeight, float amount, Cell... neighbors) {
        for (Cell neighbor : neighbors) {
            // Rút bớt chiều cao của ô cao hơn và chuyển lượng đất đó thành trầm tích ở đáy
            if (!neighbor.erosionMask && neighbor.height > centerHeight) {
                neighbor.height -= amount * 0.25f;
                neighbor.heightErosion += amount * 0.25f;
            }
        }
    }

    public static IntFunction<LandSlide> factory(GeneratorContext context) {
        return (mapSize) -> new LandSlide(mapSize, context.stabilityNoise, 0.25f, Modifier.DEFAULT);
    }
}