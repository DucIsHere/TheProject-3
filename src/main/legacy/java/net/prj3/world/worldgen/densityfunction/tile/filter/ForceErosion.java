package net.prj3.world.worldgen.densityfunction.tile.filter;

import java.util.function.IntFunction;

import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.data.worldgen.preset.settings.FilterSettings;

public class ForceErosion implements Filter {
    private final int mapSize;
    // Faulting - Cannyon
    private final Noise faultNoise;
    private final float faultDepth;
    private final float faultThreshold;
    private final float faultPower;

    // Orogeny - Uplift
    private final Noise upliftNoise;
    private final float upliftHeight;
    private final float upliftThreshold;

    private final float iterations;

    private final Modifier modifier;

    public ForceErosion(int mapSize, Noise faultNoise, float faultDepth, float faultThreshold, float faultPower,
                    Noise upliftNoise, float upliftHeight, float upliftThreshold, float iterations, Modifier modifier
    ) {
        this.mapSize = mapSize;
        this.faultNoise = faultNoise;
        this.faultDepth = faultDepth;
        this.faultPower = faultPower;
        this.faultThreshold = faultThreshold;
        this.upliftNoise = upliftNoise;
        this.upliftHeight = upliftHeight;
        this.upliftThreshold = upliftThreshold;
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

        for (int z = 0; z < total; z++) {
            for (int x = 0; x < total; x++) {
                Cell cell = map.getCellRaw(x, z);

                float wx = worldX + x;
                float wz = worldZ + z;

                float faultVal = this.faultNoise.compute(wx, wz, seedX);
                float ridgeFault = 1.0 - Math.abs(faultVal);

                if (ridgeFault > this.faultThreshold) {
                    float intensity = (ridgeFault - this.faultThreshold) / (1.0 - this.faultThreshold);
                    float displacement = (float) Math.pow(intensity, this.faultPower) * this.faultPower;

                    displacement = this.modifier.modify(cell, displacement);

                    cell.height -= displacement;
                    cell.heightErosion += displacement;

                    if (intensity > 0.4F) {
                        cell.erosionMask = true;
                    }
                }

                float upliftVal = this.upliftNoise.compute(wx, wz, seedZ);
                float ridgeUplift = 1.0 - Math.abs(upliftVal);

                if (ridgeUplift > this.upliftThreshold) {
                    float intensity = (ridgeUplift - this.upliftThreshold) / (1.0f - this.upliftThreshold);
                    float smoothIntensity = intensity * intensity * (3.0f - 2.0f * intensity);
                    float elevation = smoothIntensity * this.upliftHeight;

                    elevation = this.modifier.modify(cell, elevation);

                    cell.height += elevation;
                    cell.sediment = Math.max(0.0f, cell.sediment - (elevation * 0.1f));
                }
            }
        }
    }

    public static IntFunction<ForceErosion> factory(GeneratorContext context) {
        return (size) -> new ForceErosion(size,
            context.faultNoise, 15.0F, 0.8F, 3.0F, // Thông số Đứt gãy
            context.upliftNoise, 35.0F, 0.6F,      // Thông số Nâng kiến tạo
            Modifier.DEFAULT
        );
    }
}