package net.prj3.world.worldgen.cell;

public interface CellPopulator {
  void apply(Cell cell, float x, float z);
}
