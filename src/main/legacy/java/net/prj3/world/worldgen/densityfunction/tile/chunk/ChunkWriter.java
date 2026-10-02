package net.prj3.world.worldgen.densityfunction.tile.chunk;

import net.prj3.world.worldgen.cell.Cell;

public interface ChunkWriter extends ChunkHolder {
    Cell genCell(int x, int z);
}
