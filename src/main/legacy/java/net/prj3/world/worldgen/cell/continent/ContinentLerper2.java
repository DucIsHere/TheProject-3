package net.prj3.world.worldgen.cell.continent;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellPopulator;
import net.prj3.world.worldgen.noise.NoiseUtil;
import net.prj3.world.worldgen.noise.function.Interpolation;

public class ContinentLerper2 implements CellPopulator {
	private CellPopulator lower;
    private CellPopulator upper;
    private Interpolation interpolation;
    private float blendLower;
    private float blendUpper;
    private float blendRange;
    
    public ContinentLerper2(CellPopulator lower, CellPopulator upper, float min, float max) {
        this(lower, upper, min, max, Interpolation.LINEAR);
    }
    
    public ContinentLerper2(CellPopulator lower, CellPopulator upper, float min, float max, Interpolation interpolation) {
        this.lower = lower;
        this.upper = upper;
        this.interpolation = interpolation;
        this.blendLower = min;
        this.blendUpper = max;
        this.blendRange = this.blendUpper - this.blendLower;
    }
    
    @Override
    public void apply(Cell cell, float x, float y) {
        if (cell.continentEdge < this.blendLower) {
            this.lower.apply(cell, x, y);
            return;
        }
        if (cell.continentEdge > this.blendUpper) {
            this.upper.apply(cell, x, y);
            return;
        }
        float alpha = this.interpolation.apply((cell.continentEdge - this.blendLower) / this.blendRange);
        this.lower.apply(cell, x, y);
        float lowerVal = cell.height;
        this.upper.apply(cell, x, y);
        float upperVal = cell.height;
        cell.height = NoiseUtil.lerp(lowerVal, upperVal, alpha);

		if (cell.continentEdge > 0) {
			float autoBase = 0.12F + (cell.continentEdge * 0.08F);
			cell.Height += autoBase;
		}
    }
}
