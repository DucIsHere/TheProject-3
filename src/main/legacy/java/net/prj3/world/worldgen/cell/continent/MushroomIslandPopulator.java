package net.prj3.world.worldgen.cell.continent;

import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellPopulator;
import net.prj3.world.worldgen.noise.NoiseUtil;

public record MushroomIslandPopulator(CellPopulator ocean, CellPopulator coast, CellPopulator land, float coastPoint, float inlandPoint) implements CellPopulator {
	public static final float DEFAULT_INLAND_POINT = 0.0F;
	public static final float DEFAULT_COAST_POINT = DEFAULT_INLAND_POINT + 0.1F;
	
	@Override
	public void apply(Cell cell, float x, float z) {
		if(cell.continentEdge < this.inlandPoint) {
			this.land.apply(cell, x, z);
			return;
		}
		
		if(cell.continentEdge < this.coastPoint) {
			this.coast.apply(cell, x, z);
			float coastHeight = cell.height;
			
			this.land.apply(cell, x, z);
			float landHeight = cell.height;
			
			cell.height = NoiseUtil.lerp(landHeight, coastHeight, NoiseUtil.map(cell.continentEdge, 0.0F, 1.0F, this.inlandPoint, this.coastPoint));
			return;
		}
		
		this.ocean.apply(cell, x, z);
	}
}
