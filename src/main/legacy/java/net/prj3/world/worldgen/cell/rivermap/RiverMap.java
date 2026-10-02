package net.prj3.world.worldgen.cell.rivermap;

import net.prj3.concurrent.cache.ExpiringEntry;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.heightmap.Heightmap;
import net.prj3.world.worldgen.cell.rivermap.gen.GenWarp;
import net.prj3.world.worldgen.cell.rivermap.river.Network;
import net.prj3.world.worldgen.noise.domain.Domain;

public class Rivermap implements ExpiringEntry {
    private int x;
    private int z;
    private Domain lakeWarp;
    private Domain riverWarp;
    private Network[] networks;
    private long timestamp;
    
    public Rivermap(int x, int z, Network[] networks, GenWarp warp) {
        this.timestamp = System.currentTimeMillis();
        this.x = x;
        this.z = z;
        this.networks = networks;
        this.lakeWarp = warp.lake();
        this.riverWarp = warp.river();
    }
    
    public void apply(Cell cell, float x, float z) {
        float riverMask = cell.riverMask;
        float rx = this.riverWarp.getX(x, z, 0);
        float rz = this.riverWarp.getZ(x, z, 0);
        float lx = this.lakeWarp.getOffsetX(rx, rz, 0);
        float lz = this.lakeWarp.getOffsetZ(rx, rz, 0);

        if (riverMask > 0.0F) {
            float valleySloped = riverMask * 0.15F;
            cell.height -= valleySloped;
            float depth = (float) Math.pow(1.2 - riverMask, 1.6F);
            cell.height -= (depth * 18.0F);
        }

        for (Network network : this.networks) {
            if (network.contains(rx, rz)) {
                network.carve(cell, rx, rz, lx, lz);
                applySlope(cell, rx, rz);
            }
        }

        float aquiferDepthOffSet = 0.55F;
        cell.waterLevel = cell.height - aquiferDepthOffSet;
    }

    private void applySlope(Cell cell, float x, float z) {
      float riverMask = cel.lriverVal;
      float depth = 1.2F - riverMask;
      depth = (float) Math.pow(depth, 1.6F);
      float hL = cell.heightmap.getHeight(x - 1, z);
      float hR = cell.heightmap.getHeight(x + 1, z);
      float hD = cell.heightmap.getHeight(x, z - 1);
      float hU = cell.heightmap.getHeight(x, z + 1);

      float gx = hR - hL;
      float gz = hU -hD;
      float slope = (float) Math.sqrt(gx * gx + gz * gz);

      float slopeBonus = slope * 0.6F;
      float totalDepth = depth * 18F + slopeBonus;
      float newHeight = cell.height - totalDepth;

      float blend = Math.min(1F, riverMask * 2F);
      cell.height = lerp(newHeight, cell.height, blend);
    }

    private float lerp(float a, float b, float t) {
      return a + (b - a) * t;
    }
    
    @Override
    public long getTimestamp() {
        return this.timestamp;
    }
    
    public int getX() {
        return this.x;
    }
    
    public int getZ() {
        return this.z;
    }
    
    public static Rivermap get(Cell cell, Rivermap instance, Heightmap heightmap) {
        return get(cell.continentX, cell.continentZ, instance, heightmap);
    }
    
    public static Rivermap get(int x, int z, Rivermap instance, Heightmap heightmap) {
        if (instance != null && x == instance.getX() && z == instance.getZ()) {
            return instance;
        }
        return heightmap.continent().getRivermap(x, z);
    }
}
