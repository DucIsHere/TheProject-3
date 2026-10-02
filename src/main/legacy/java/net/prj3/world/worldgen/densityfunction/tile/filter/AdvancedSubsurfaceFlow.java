package net.prj3.world.worldgen.densityfunction.tile.filter;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.util.FastRandom;
import net.prj3.world.worldgen.noise.NoiseUtil;

import java.util.Arrays;
import java.util.function.IntFunction;

/**
 * ULTIMATE SUBSURFACE FLOW & RIVER SCULPTOR (Bản tối ưu hóa cao nhất)
 * Mô phỏng Thủy văn đa tầng, Nhiệt đối lưu, Vận chuyển khoáng chất và Kiến tạo Karst chạy dài.
 */
public class AdvancedSubsurfaceFlow implements Filter {
    private final int mapSize;
    private final float seaLevel;
    private final float baseViscosity;
    private final float solubility;
    private final float riverThreshold;
    private final float flowMomentum;
    private final float infiltrationRate;
    private final float int iterations;

    private final Noise permeabilityNoise;
    private final Noise thermalNoise;

    public AdvancedSubsurfaceFlow(int mapSize, float seaLevel, float baseViscosity, float solubility,
                                  float riverThreshold, float flowMomentum, float infiltrationRate, int iterations) {
        this.mapSize = mapSize;
        this.seaLevel = context.levels.water;
        this.baseViscosity = baseViscosity;
        this.solubility = solubility;
        this.riverThreshold = riverThreshold;
        this.flowMomentum = flowMomentum;
        this.infiltrationRate = infiltrationRate;
        this.iterations iterations;
        this.permeabilityNoise = context.stabilityNoise;
        this.thermalNoise = context.continentalnessNoise;
    }

    @Override
    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int seedX, int seedZ, int iterations) {
        final Size size = map.getBlockSize();
        final int width = size.width();
        final int total = size.total();

        // Mảng đệm Double Buffering để xử lý luồng (Flux)
        float[] waterFlux = new float[total];
        float[] thermalFlux = new float[total];
        float[] soluteFlux = new float[total];
        float[] flowAccumulation = new float[total];
        float[] currentThermal = new float[total];

        // Lấy số bước lặp từ cấu hình
        int simSteps = advancedSubsurfaceFlow.iterations() > 0 ? advancedSubsurfaceFlow.iterations() : iterations;

        for (int iter = 0; iter < simSteps; iter++) {
            Arrays.fill(waterFlux, 0.0f);
            Arrays.fill(thermalFlux, 0.0f);
            Arrays.fill(soluteFlux, 0.0f);
            Arrays.fill(flowAccumulation, 0.0f);

            // BƯỚC 1: PHÂN TÍCH MÔI TRƯỜNG & VẬT LÝ ĐỊA PHƯƠNG
            for (int i = 0; i < total; i++) {
                Cell cell = map.getCellRaw(i);
                int wx = map.getBlockX() + (i % width);
                int wz = map.getBlockZ() + (i / width);

                // 1.1 Infiltration: Thấm dựa trên Scale và Power từ Settings
                float localPerm = (float) permeabilityNoise.compute(wx * advancedSubsurfaceFlow.soilPermeabilityScale(), wz * advancedSubsurfaceFlow.soilPermeabilityScale(), seedX);
                float infPotential = cell.precipitation * (localPerm * advancedSubsurfaceFlow.soilPermeabilityPower());
                float absorbed = Math.min(cell.water * advancedSubsurfaceFlow.infiltrationRate(), (cell.aquiferCapacity - cell.aquiferWater) * infPotential);
                cell.water -= absorbed;
                cell.aquiferWater += absorbed;

                // 1.2 Geothermal: Tính toán nhiệt độ với Intensity và Gradient
                float heatSource = (float) thermalNoise.compute(wx * s.thermalNoiseScale(), wz * s.thermalNoiseScale(), seedX);
                currentThermal[i] = (cell.height < 0 ? Math.abs(cell.height) * s.geothermalGradient() : 0) + (heatSource * s.thermalNoiseIntensity());
                
                // Hiệu ứng giãn nở do nhiệt (Density Thermal Scale)
                float densityEffect = 1.0f - (currentThermal[i] * s.densityThermalScale() * 0.001f);

                // 1.3 Hydraulic Head: Áp suất thực tế có tính đến hằng số Gravity
                float confinement = (1.0f - localPerm) * s.confinedPressureFactor();
                cell.aquiferPressure = (cell.height * s.gravity()) + 
                                      (cell.aquiferWater / Math.max(1.0f, cell.aquiferCapacity)) * (10.0f + confinement) * densityEffect;
            }

            // BƯỚC 2: MÔ PHỎNG DÒNG CHẢY ĐA HƯỚNG (Tạo xương sống cho sông ngầm)
            for (int z = 1; z < width - 1; z++) {
                for (int x = 1; x < width - 1; x++) {
                    simulatePhysics(map, x, z, width, waterFlux, thermalFlux, soluteFlux, flowAccumulation, currentThermal);
                }
            }

            // BƯỚC 3: KIẾN TẠO ĐỊA HÌNH (Sông chạy dài & sâu)
            for (int i = 0; i < total; i++) {
                Cell cell = map.getCellRaw(i);
                
                cell.aquiferWater += waterFlux[i];
                cell.aquiferSolutes += soluteFlux[i];
                cell.temperature += thermalFlux[i] * s.thermalAdvection();

                // CƠ CHẾ SCULPTING (Bào mòn Karst dựa trên Threshold và Momentum)
                float flowPower = flowAccumulation[i];
                if (flowPower > s.riverThreshold()) {
                    // Tốc độ bào mòn dựa trên hằng số Solubility
                    float erosion = flowPower * s.solubility() * (1.0f + cell.temperature * 0.1f);
                    
                    // Đào sâu túi nước và hạ thấp địa tầng ngầm (Carving Strength)
                    cell.aquiferCapacity += erosion * 5.0f;
                    cell.heightErosion += erosion * s.carvingStrength();
                    
                    // MOMENTUM: Tạo áp suất âm tại lòng sông để kéo nước đi xa hơn
                    cell.aquiferPressure -= s.flowMomentum() * flowPower;
                    cell.aquiferSolutes += erosion * 0.4f;
                }

                // LẮNG ĐỌNG KHOÁNG SẢN (Deposition)
                float saturation = cell.aquiferCapacity * 0.4f;
                if (cell.aquiferSolutes > saturation) {
                    float deposit = (cell.aquiferSolutes - saturation) * s.sedimentDeposition();
                    cell.aquiferSolutes -= deposit;
                    cell.aquiferCapacity -= deposit * 0.1f;
                }

                // PHUN TRÀO (Resurgence Threshold)
                if (cell.aquiferWater > cell.aquiferCapacity * s.resurgenceThreshold()) {
                    float surge = cell.aquiferWater - cell.aquiferCapacity;
                    cell.water += surge;
                    cell.aquiferWater = cell.aquiferCapacity;
                    
                    // Nếu là nước nóng -> Đánh dấu là Biome suối nước nóng
                    if (cell.temperature > 35.0f) cell.biomeStyle = 1.0f; 
                }
            }
        }
    }

    private void simulatePhysics(Filterable map, int x, int z, int width, float[] wFlux, float[] tFlux, float[] sFlux, float[] fAcc, float[] tMap) {
        int idx = z * width + x;
        Cell curr = map.getCellRaw(idx);
        if (curr.aquiferWater <= 0.02f) return;

        // Thuật toán dốc nhất (Steepest Descent) để gom nước thành dòng chảy tập trung
        int bestN = -1;
        float maxGrad = 0;

        for (int dz = -1; dz <= 1; dz++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dz == 0) continue;
                int nIdx = (z + dz) * width + (x + dx);
                Cell neighbor = map.getCellRaw(nIdx);
                
                float dist = (dx == 0 || dz == 0) ? 1.0f : 1.414f;
                float gradient = (curr.aquiferPressure - neighbor.aquiferPressure) / dist;

                if (gradient > maxGrad) {
                    maxGrad = gradient;
                    bestN = nIdx;
                }
            }
        }

        if (bestN != -1) {
            // Độ nhớt thực tế (Viscosity) và Hệ số dẫn thủy lực (Hydraulic Conductivity)
            float viscosity = s.baseViscosity() / (1.0f + tMap[idx] * 0.025f);
            float flow = maxGrad * s.hydraulicConductivity() * (1.0f / viscosity);
            flow = Math.min(flow, curr.aquiferWater * 0.35f);

            // Vận chuyển khối lượng nước
            wFlux[bestN] += flow;
            wFlux[idx] -= flow;

            // Vận chuyển nhiệt năng
            float heat = flow * tMap[idx];
            tFlux[bestN] += heat;
            tFlux[idx] -= heat;

            // Vận chuyển chất tan
            float conc = curr.aquiferSolutes / Math.max(0.1f, curr.aquiferWater);
            float solutesMoved = flow * conc;
            sFlux[bestN] += solutesMoved;
            sFlux[idx] -= solutesMoved;

            // Tích tụ dòng chảy (Key để đào sông chạy dài)
            fAcc[bestN] += flow;
        }
    }

    public static IntFunction<AdvancedSubsurfaceFlow> factory(GeneratorContext context) {
        return (size) -> new AdvancedSubsurfaceFlow(size, context);
    }
}
