package net.prj3.world.worldgen.cell;

import net.prj3.concurrent.Resource;
import net.prj3.concurrent.SimpleResource;
import net.prj3.concurrent.pool.ThreadLocalPool;
import net.prj3.world.worldgen.cell.biome.type.BiomeType;
import net.prj3.world.worldgen.cell.terrain.Terrain;
import net.prj3.world.worldgen.cell.terrain.TerrainType;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;

public class Cell {
    public Terrain terrain;
    public BiomeType biome;

    public static final GroupLayout LAYOUT = MemoryLayout.structLayout(
            ValueLayout.JAVA_FLOAT.withName("height"),
            ValueLayout.JAVA_FLOAT.withName("heightErosion"),
            ValueLayout.JAVA_FLOAT.withName("sediment"),
            ValueLayout.JAVA_FLOAT.withName("gradient"),
            ValueLayout.JAVA_FLOAT.withName("regionMoisture"),
            ValueLayout.JAVA_FLOAT.withName("regionTemperature"),
            ValueLayout.JAVA_FLOAT.withName("continentId"),
            ValueLayout.JAVA_FLOAT.withName("continentEdge"),
            ValueLayout.JAVA_FLOAT.withName("terrainRegionId"),
            ValueLayout.JAVA_FLOAT.withName("terrainRegionEdge"),
            ValueLayout.JAVA_FLOAT.withName("biomeRegionId"),
            ValueLayout.JAVA_FLOAT.withName("biomeRegionEdge"),
            ValueLayout.JAVA_FLOAT.withName("macroBiomeId"),
            ValueLayout.JAVA_FLOAT.withName("riverMask"),
            ValueLayout.JAVA_INT.withName("continentX"),
            ValueLayout.JAVA_INT.withName("continentZ"),
            ValueLayout.JAVA_BOOLEAN.withName("erosionMask"),
            MemoryLayout.paddingLayout(24), // Alignment padding (3 bytes)
            ValueLayout.JAVA_FLOAT.withName("erosion"),
            ValueLayout.JAVA_FLOAT.withName("weirdness"),
            ValueLayout.JAVA_FLOAT.withName("temperature"),
            ValueLayout.JAVA_FLOAT.withName("moisture"),
            ValueLayout.JAVA_FLOAT.withName("damage"),
            ValueLayout.JAVA_FLOAT.withName("beachNoise")
    );

    private static final long HEIGHT_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("height"));
    private static final long HEIGHT_EROSION_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("heightErosion"));
    private static final long SEDIMENT_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("sediment"));
    private static final long GRADIENT_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("gradient"));
    private static final long REGION_MOISTURE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("regionMoisture"));
    private static final long REGION_TEMP_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("regionTemperature"));
    private static final long CONTINENT_ID_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("continentId"));
    private static final long CONTINENT_EDGE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("continentEdge"));
    private static final long TERRAIN_REG_ID_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("terrainRegionId"));
    private static final long TERRAIN_REG_EDGE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("terrainRegionEdge"));
    private static final long BIOME_REG_ID_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("biomeRegionId"));
    private static final long BIOME_REG_EDGE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("biomeRegionEdge"));
    private static final long MACRO_BIOME_ID_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("macroBiomeId"));
    private static final long RIVER_MASK_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("riverMask"));
    private static final long CONTINENT_X_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("continentX"));
    private static final long CONTINENT_Z_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("continentZ"));
    private static final long EROSION_MASK_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("erosionMask"));
    private static final long EROSION_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("erosion"));
    private static final long WEIRDNESS_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("weirdness"));
    private static final long SHARPNESS_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("sharpness"));
    private static final long TEMPERATURE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("temperature"));
    private static final long MOISTURE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("moisture"));
    private static final long DAMAGE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("damage"));
    private static final long BEACH_NOISE_OFFSET = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("beachNoise"));

    private static final Cell DEFAULTS = new Cell();
    private static final Cell EMPTY = new Cell() {
        @Override
        public boolean isAbsent() {
            return true;
        }
    };

    private static final ThreadLocalPool<Cell> POOL = new ThreadLocalPool<>(32, Cell::new, Cell::reset);
    public static final ThreadLocal<Resource<Cell>> LOCAL = ThreadLocal.withInitial(() -> new SimpleResource<>(new Cell<>, Cell::reset));

    private final MemorySegment segment;

    public Cell() {
        this.segment = MemorySegement.allocateNative(LAYOUT.bizeSize());
        resetToDefaults();
    }

    public Cell() {
        this.segment = segment;
        resetToDefaults();
    }

    public MemorySegment segment() {
        return this.segment;
    }

    private void resetToDefaults() {
        setRegionMoisture(0.5F);
        setRegionTemperature(0.5F);
        setBiomeRegionEdge(1.0F);
        setRiverMask(1.0F);
        setErosionMask(false);
    }

    public float getHeight() { return segment.get(ValueLayout.JAVA_FLOAT, HEIGHT_OFFSET); }
    public void setHeight(float val) { segment.set(ValueLayout.JAVA_FLOAT, HEIGHT_OFFSET, val); }

    public float getHeightErosion() { return segment.get(ValueLayout.JAVA_FLOAT, HEIGHT_EROSION_OFFSET); }
    public void setHeightErosion(float val) { segment.set(ValueLayout.JAVA_FLOAT, HEIGHT_EROSION_OFFSET, val); }

    public float getSediment() { return segment.get(ValueLayout.JAVA_FLOAT, SEDIMENT_OFFSET); }
    public void setSediment(float val) { segment.set(ValueLayout.JAVA_FLOAT, SEDIMENT_OFFSET, val); }

    public float getGradient() { return segment.get(ValueLayout.JAVA_FLOAT, GRADIENT_OFFSET); }
    public void setGradient(float val) { segment.set(ValueLayout.JAVA_FLOAT, GRADIENT_OFFSET, val); }

    public float getRegionMoisture() { return segment.get(ValueLayout.JAVA_FLOAT, REGION_MOISTURE_OFFSET); }
    public void setRegionMoisture(float val) { segment.set(ValueLayout.JAVA_FLOAT, REGION_MOISTURE_OFFSET, val); }

    public float getRegionTemperature() { return segment.get(ValueLayout.JAVA_FLOAT, REGION_TEMP_OFFSET); }
    public void setRegionTemperature(float val) { segment.set(ValueLayout.JAVA_FLOAT, REGION_TEMP_OFFSET, val); }

    public float getContinentId() { return segment.get(ValueLayout.JAVA_FLOAT, CONTINENT_ID_OFFSET); }
    public void setContinentId(float val) { segment.set(ValueLayout.JAVA_FLOAT, CONTINENT_ID_OFFSET, val); }

    public float getContinentEdge() { return segment.get(ValueLayout.JAVA_FLOAT, CONTINENT_EDGE_OFFSET); }
    public void setContinentEdge(float val) { segment.set(ValueLayout.JAVA_FLOAT, CONTINENT_EDGE_OFFSET, val); }

    public float getTerrainRegionId() { return segment.get(ValueLayout.JAVA_FLOAT, TERRAIN_REG_ID_OFFSET); }
    public void setTerrainRegionId(float val) { segment.set(ValueLayout.JAVA_FLOAT, TERRAIN_REG_ID_OFFSET, val); }

    public float getTerrainRegionEdge() { return segment.get(ValueLayout.JAVA_FLOAT, TERRAIN_REG_EDGE_OFFSET); }
    public void setTerrainRegionEdge(float val) { segment.set(ValueLayout.JAVA_FLOAT, TERRAIN_REG_EDGE_OFFSET, val); }

    public float getBiomeRegionId() { return segment.get(ValueLayout.JAVA_FLOAT, BIOME_REG_ID_OFFSET); }
    public void setBiomeRegionId(float val) { segment.set(ValueLayout.JAVA_FLOAT, BIOME_REG_ID_OFFSET, val); }

    public float getBiomeRegionEdge() { return segment.get(ValueLayout.JAVA_FLOAT, BIOME_REG_EDGE_OFFSET); }
    public void setBiomeRegionEdge(float val) { segment.set(ValueLayout.JAVA_FLOAT, BIOME_REG_EDGE_OFFSET, val); }

    public float getMacroBiomeId() { return segment.get(ValueLayout.JAVA_FLOAT, MACRO_BIOME_ID_OFFSET); }
    public void setMacroBiomeId(float val) { segment.set(ValueLayout.JAVA_FLOAT, MACRO_BIOME_ID_OFFSET, val); }

    public float getRiverMask() { return segment.get(ValueLayout.JAVA_FLOAT, RIVER_MASK_OFFSET); }
    public void setRiverMask(float val) { segment.set(ValueLayout.JAVA_FLOAT, RIVER_MASK_OFFSET, val); }

    public int getContinentX() { return segment.get(ValueLayout.JAVA_INT, CONTINENT_X_OFFSET); }
    public void setContinentX(int val) { segment.set(ValueLayout.JAVA_INT, CONTINENT_X_OFFSET, val); }

    public int getContinentZ() { return segment.get(ValueLayout.JAVA_INT, CONTINENT_Z_OFFSET); }
    public void setContinentZ(int val) { segment.set(ValueLayout.JAVA_INT, CONTINENT_Z_OFFSET, val); }

    public boolean isErosionMask() { return segment.get(ValueLayout.JAVA_BOOLEAN, EROSION_MASK_OFFSET); }
    public void setErosionMask(boolean val) { segment.set(ValueLayout.JAVA_BOOLEAN, EROSION_MASK_OFFSET, val); }

    public float getErosion() { return segment.get(ValueLayout.JAVA_FLOAT, EROSION_OFFSET); }
    public void setErosion(float val) { segment.set(ValueLayout.JAVA_FLOAT, EROSION_OFFSET, val); }

    public float getWeirdness() { return segment.get(ValueLayout.JAVA_FLOAT, WEIRDNESS_OFFSET); }
    public void setWeirdness(float val) { segment.set(ValueLayout.JAVA_FLOAT, WEIRDNESS_OFFSET, val); }

    public float getSharpness() { return segment.get(ValueLayout.JAVA_FLOAT, SHARPNESS_OFFSET); }
    public void setSharpness(float val) { segment.set(ValueLayout.JAVA_FLOAT, SHARPNESS_OFFSET, val); }

    public float getTemperature() { return segment.get(ValueLayout.JAVA_FLOAT, TEMPERATURE_OFFSET); }
    public void setTemperature(float val) { segment.set(ValueLayout.JAVA_FLOAT, TEMPERATURE_OFFSET, val); }

    public float getMoisture() { return segment.get(ValueLayout.JAVA_FLOAT, MOISTURE_OFFSET); }
    public void setMoisture(float val) { segment.set(ValueLayout.JAVA_FLOAT, MOISTURE_OFFSET, val); }

    public float getDamage() { return segment.get(ValueLayout.JAVA_FLOAT, DAMAGE_OFFSET); }
    public void setDamage(float val) { segment.set(ValueLayout.JAVA_FLOAT, DAMAGE_OFFSET, val); }

    @Deprecated(forRemoval = true)
    public float getBeachNoise() { return segment.get(ValueLayout.JAVA_FLOAT, BEACH_NOISE_OFFSET); }

    @Deprecated(forRemoval = true)
    public void setBeachNoise(float val) { segment.set(ValueLayout.JAVA_FLOAT, BEACH_NOISE_OFFSET, val); }

    public void copyFrom(Cell other) {
        this.segment.copyFrom(other.segment);
    }

    public Cell reset() {
        this.copyFrom(Cell.DEFAULTS);
        return this;
    }

    @Deprecated(forRemoval = true)
    public boolean isAbsent() {
        return false;
    }

    @Deprecated(forRemoval = true)
    public static Cell empty() {
        return Cell.EMPTY;
    }

    public static Resource<Cell> getResource() {
        Resource<Cell> resource = Cell.LOCAL.get();
        if (resource.isOpen()) {
            return Cell.POOL.get();
        }
        return resource;
    }

    public interface Visitor {
        void visit(Cell cell, int x, int z);
    }
}
