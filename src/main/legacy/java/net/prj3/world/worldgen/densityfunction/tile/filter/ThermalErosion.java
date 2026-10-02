package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.data.worldgen.preset.settings.FilterSettings

public class ThermalErosion implements Filter {
    private final int mapSize;
    private final float talusThreshold;
    private final float materialTransfer;
    private finap float iterations;
    private final Modifier modifier;

    public ThermalErosion(int mapSize, float talusThreshold, float materialTransfer, float iterations, Modifier modifier) {
        this.mapSize = mapSize;
        this.talusThreshold = talusThreshold;
        this.materialTransfer = materialTransfer;
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

        float[] deltaHeight = new float[total * total];

        for (int iter = 0; iter < iterations; iter++) {
            for (int x = 1; x < total - 1; x++) {
                int index = z * total + x;
                Cell cell = map.getCellRaw(x, z);

                float h = cell.height;

                checkNeighbor(map, x + 1, z, h, index, deltaHeight, cell);
                checkNeighbor(map, x - 1, z, h, index, deltaHeight, cell);
                checkNeighbor(map, x, z + 1, h, index, deltaHeight, cell);
                checkNeighbor(map, x, z - 1, h, index, deltaHeight, cell);
            }
        }

        for (int i = 0; i < deltaHeight.length; i++) {
            if (deltaHeight[i] != 0) {
                int cx = i % total;
                int cz = i / total;

                Cell c = map.getCellRaw(cx, cz);

                c.height += deltaHeight[i];

                if (deltaHeight[i] > 0) {
                    c.sediment += deltaHeight[i];
                } else {
                    c.heightErosion -= deltaHeight[i];
                    c.sediment = Math.max(0, c.sediment + c.deltaHeight[i]);
                }
            }
        }
    }

    private void checkNeighbor(Filterable map, int nx, int nz, float h, int selfIdx, float[] deltaHeight, Cell cell) {
        Cell neighbor = map.getCellRaw(nx, nz);
        float nh = neighbor.height;
        float diff = h - nh;

        // Nếu độ dốc vượt quá ngưỡng Talus
        if (diff > this.talusThreshold) {
            // Lượng vật chất dư thừa cần di chuyển
            float excess = diff - this.talusThreshold;
            
            // Tính toán lượng dịch chuyển dựa trên transferRate và Modifier (độ cứng của đất/đá)
            float moveAmount = excess * this.transferRate;
            moveAmount = this.modifier.modify(cell, moveAmount);

            // Cập nhật vào mảng delta (Trừ ở ô cao, cộng vào ô thấp)
            deltaHeight[selfIdx] -= moveAmount;
            deltaHeight[nz * map.getBlockSize().total() + nx] += moveAmount;
        }
    }
    /**
     * Factory method để khởi tạo nhanh với các thông số chuẩn địa chất
     **/
    public static IntFunction<ThermalErosion> factory(GeneratorContext context) {
        return (mapSize) -> new ThermalErosion(mapSize, 0.15f, 0.5f, Modifier.DEFAULT);
    }
}