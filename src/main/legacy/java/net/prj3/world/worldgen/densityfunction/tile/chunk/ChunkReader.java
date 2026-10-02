package net.prj3.world.worldgen.densityfunction.tile.chunk;

import net.prj3.concurrent.Disposable;
import net.prj3.world.worldgen.cell.Cell;

public interface ChunkReader extends ChunkHolder, Disposable {
    Cell getCell(int x, int z);
}
