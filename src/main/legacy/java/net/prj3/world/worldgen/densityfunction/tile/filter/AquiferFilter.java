package net.prj3.world.worldgen.densityfunction.tile.filter;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.NoiseUtil;
import net.prj3.world.worldgen.noise.module.Noises;

import java.util.Arrays;
import java.util.function.IntFunction;

/**
 * AquiferFilter - Mô phỏng hệ thống túi nước ngầm và tầng ngậm nước (Aquifers)
 * Dựa trên nguyên lý Darcy về dòng chảy trong môi trường xốp và áp suất thủy tĩnh.
 */
public class AquiferFilter implements Filter {
    private final int mapSize;
    private final Size size;
    private final float seaLevel;
    private final Noise stabilityNoise;
    private final float iterations;
    private final float basePorosity;
    private final float permeabilityScale;
    private final float rechargeRate;
    private final float pressureFactor;

    // Các hằng số vật lý mô phỏng
    private static final float GRAVITY = 9.81f;
    private static final float WATER_DENSITY = 1.0f;

    public AquiferFilter(int mapSize, Size size, float seaLevel, float iterations,
                         float basePorosity, float permeabilityScale, float rechargeRate, float pressureFactor,
                         Noise stabilityNoise) {
        this.size = size;
        this.mapSize = mapSize();
        this.seaLevel = context.levels.water;
        this.stabilityNoise = context.stabilityNoise;
        this.iterations = iterations;
        this.basePorosity = basePorosity;
        this.permeabilityScale = permeabilityScale;
        this.rechargeRate = rechargeRate;
        this.pressureFactor = pressureFactor;
    }

    public int getSize() {
        return mapSize;
    }

    @Override
    public void apply(Filterable map, int seedX, int seedZ, int iterations) {
        int totalCells = size.total();
        
        // Bước 1: Khởi tạo các đặc tính địa tầng (Lithology)
        // Sử dụng Arrays.parallelSetAll nếu map hỗ trợ truy cập mảng để tối ưu CPU đa nhân
        for (int i = 0; i < totalCells; i++) {
            Cell cell = map.getCellRaw(i);
            
            // Tính toán độ rỗng (Porosity) dựa trên độ ổn định địa chất
            // Stability thấp (~0) thường là vùng đá vôi (Karst) hoặc đất xốp -> Sức chứa cao
            float stability = getNoiseValue(stabilityNoise, i);
            float localPorosity = (1.0f - stability) * settings.basePorosity();
            
            // Áp dụng định luật nén: Càng xuống sâu (height thấp), lỗ rỗng càng bị nén lại
            float depthEffect = NoiseUtil.clamp(cell.height, 0.1f, 1.0f);
            cell.aquiferCapacity = localPorosity * depthEffect * 50.0f; // Đơn vị giả định: m3/cell

            // Bước 2: Nạp nước ban đầu (Initial Recharge)
            // Lượng nước ban đầu phụ thuộc vào độ ẩm (Moisture) và vị trí so với mực nước biển
            float moistureRecharge = cell.moisture * settings.rechargeRate();
            if (cell.height < seaLevel) {
                // Vùng dưới biển hoặc sông luôn bão hòa nước
                cell.aquiferWater = cell.aquiferCapacity;
            } else {
                // Vùng cao: Nước tích tụ dựa trên lượng mưa (moisture) và khả năng giữ nước
                cell.aquiferWater = cell.aquiferCapacity * moistureRecharge;
            }
        }

        // Bước 3: Mô phỏng dòng chảy ngầm (Groundwater Flow)
        // Sử dụng toán học: Nước chảy từ nơi có áp suất cao (P = rho * g * h) sang nơi thấp
        if (settings.simulateFlow()) {
            simulateHydraulicHead(map, iterations);
        }
    }

    /**
     * Mô phỏng sự cân bằng áp suất giữa các cell lân cận
     */
    private void simulateHydraulicHead(Filterable map, int iterations) {
        int width = size.width();
        int height = size.height();

        for (int iter = 0; iter < iterations; iter++) {
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    int idx = y * width + x;
                    Cell current = map.getCellRaw(idx);

                    // Tính "Cột nước thủy lực" (Hydraulic Head) = Cao độ thực tế + Áp suất nước ngầm
                    // H = z + (P / (rho * g))
                    float currentHead = current.height + (current.aquiferWater / current.aquiferCapacity) * settings.pressureFactor();

                    // Kiểm tra 4 hướng lân cận (Von Neumann Neighborhood)
                    int[] neighbors = {idx - 1, idx + 1, idx - width, idx + width};
                    for (int nIdx : neighbors) {
                        Cell neighbor = map.getCellRaw(nIdx);
                        float neighborHead = neighbor.height + (neighbor.aquiferWater / neighbor.aquiferCapacity) * settings.pressureFactor();

                        if (currentHead > neighborHead) {
                            // Chênh lệch áp suất
                            float diff = currentHead - neighborHead;
                            // Định luật Darcy: Q = K * i (Dòng chảy = Độ thấm * Độ dốc thủy lực)
                            float flow = diff * settings.permeabilityScale() * 0.25f;

                            // Đảm bảo không rút quá lượng nước hiện có
                            flow = Math.min(flow, current.aquiferWater * 0.1f);

                            current.aquiferWater -= flow;
                            neighbor.aquiferWater += flow;
                        }
                    }
                }
            }
        }
    }

    /**
     * Factory class để khởi tạo filter thông qua hệ thống GeneratorContext
     */
    public static class Factory implements IntFunction<AquiferFilter> {
        private final GeneratorContext context;

        public Factory(GeneratorContext context) {
            this.context = context;
        }

        @Override
        public AquiferFilter apply(int sizeValue) {
            return new AquiferFilter(new Size(sizeValue), context);
        }
    }
}