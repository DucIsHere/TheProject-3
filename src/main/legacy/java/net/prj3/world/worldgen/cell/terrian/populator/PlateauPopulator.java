package net.prj3.world.worldgen.cell.terrain.populator;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellPopulator;
import net.prj3.world.worldgen.noise.NoiseUtil;

public record PlateauPopulator(
    CellPopulator source, 
    float minHeight, // Bắt đầu làm phẳng từ độ cao nào (ví dụ 0.5F)
    float maxHeight, // Giới hạn độ cao tối đa của cao nguyên (ví dụ 0.65F)
    float smoothness // Độ dốc của cạnh cao nguyên
) implements CellPopulator {

    @Override
    public void apply(Cell cell, float x, float z) {
        source.apply(cell, x, z);

        // Chỉ tác động lên đất liền và tránh làm hỏng sông (river < 0.25)
        if (cell.height > minHeight && cell.river < 0.25F) {
            float range = maxHeight - minHeight;
            float delta = cell.height - minHeight;
            
            // Sử dụng hàm SmoothStep để làm phẳng đỉnh nhưng vẫn giữ độ mượt
            float t = Math.min(1.0F, delta / range);
            float plateaued = minHeight + (range * NoiseUtil.smoothStep(t) * (1.0F - smoothness));
            
            cell.height = plateaued;
        }
    }
}
