package net.prj3.world.worldgen.cell.continent;

public interface SimpleContinent extends Continent {
  floatEdgeValue(float x, float z);

  default float getDistanceToEdge(int cx, int cz, float dx, float dz) {
    return 1.0F;
  }

  default float getDistanceToOcean(int cx, int cz, float dx, float dy) {
    return 1.0F;
  }
}
