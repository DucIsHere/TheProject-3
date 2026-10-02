package net.prj3.world.worldgen.cell.continent.simple;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.util.Seed;

public class MultiContinentGenerator extends ContinentGenerator {
    
    protected final GeneratorContext context;

    public MultiContinentGenerator(Seed seed, GeneratorContext context) {
        super(seed, context);
        this.context = context;
    }

    @Override
    public void apply(Cell cell, float x, float y) {
        var settings = context.preset.world().continent;
        
        // 1. Warp tọa độ (Sử dụng warp đã compound slider ở class cha)
        float wx = this.warp.getX(x, y, this.seed);
        float wy = this.warp.getY(x, y, this.seed);
        float px = wx * this.frequency;
        float py = wy * this.frequency;

        int xi = NoiseUtil.floor(px);
        int yi = NoiseUtil.floor(py);

        // 2. Hash dựa trên tọa độ Grid
        float hash = NoiseUtil.hash2D(xi, yi);

        // A. Continent Skipping (Slider)
        if (hash < settings.continentSkipping) {
            cell.continentEdge = 0;
            return;
        }

        // B. Size Variance (Slider)
        float currentJitter = this.offsetAlpha + (hash * settings.sizeVariance);

        // 3. Logic Voronoi tìm khoảng cách gần nhất
        float minDist = 999999F;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int cx = xi + dx;
                int cy = yi + dy;
                
                // Lấy vector ngẫu nhiên cho ô
                var vec = getVec(cx, cy); 
                
                // Áp dụng currentJitter vào đây
                float cxf = cx + vec.x() * currentJitter;
                float cyf = cy + vec.y() * currentJitter;
                
                float dist = distanceFunc.apply(px - cxf, py - cyf);
                if (dist < minDist) {
                    minDist = dist;
                    // Lưu ID và tọa độ tâm lục địa
                    cell.continentX = cx;
                    cell.continentZ = cy;
                }
            }
        }
        
        // 4. Gán giá trị cuối cùng (Map minDist về dải độ cao)
        cell.continentEdge = NoiseUtil.map(minDist, clampMin, clampMax, clampRange);
        cell.continentId = NoiseUtil.hash2D(cell.continentX, cell.continentZ);
        cell.continentEdge = NoiseUtil.map(minDist, clampMin, clampMax, clampRange);

        if (context.preset.world().general.enableSloped) {
            float vErosion = cell.riverMask * context.preset.world().terrain.plains.valleyErosion;
            float vWeird = (float)Math.sin(x * 0.05f + y * 0.05f) * context.preset.world().terrain.plains.valleyWeirdness;

            cell.height -= (vErosion + vWeird);
        }
    }
}
