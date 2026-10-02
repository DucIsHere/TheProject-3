package net.prj3.world.worldgen.cell.continent.simple;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.noise.NoiseUtil.Vec2i;
import net.prj3.world.worldgen.util.PosUtil;
import net.prj3.world.worldgen.util.Seed;

public class SingleContinentGenerator extends ContinentGenerator {
    private Vec2i center;
    
    public SingleContinentGenerator(Seed seed, GeneratorContext context) {
        super(seed, context);
        long center = this.getNearestCenter(0.0F, 0.0F);
        int cx = PosUtil.unpackLeft(center);
        int cz = PosUtil.unpackRight(center);
        this.center = new Vec2i(cx, cz);
    }
    
    @Override
public void apply(Cell cell, float x, float y) {
    // 1. Chạy logic gốc để tính toán hình dáng
    super.apply(cell, x, y);

    // 2. Kiểm tra nếu không phải đảo trung tâm thì xóa
    if (cell.continentX != this.center.x() || cell.continentZ != this.center.y()) {
        cell.continentId = 0.0F;
        cell.continentEdge = 0.0F;
        cell.continentX = 0;
        cell.continentZ = 0;
    } else {
        // --- PHẦN ADD THÊM CHO SINGLE CONTINENT ---
        // Bạn có thể dùng verticalScale từ slider để làm đảo cao hơn
        var settings = context.preset.world().continent;
        if (settings.general.enableSloped) {
            cell.waterLevel = cell.height - (settings.terrain.plains.aquiferDepthOffSet / 100.0F);
        }
        cell.height *= settings.verticalScale; 
    }
}
}
