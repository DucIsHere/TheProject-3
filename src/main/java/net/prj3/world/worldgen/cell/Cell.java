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

    public static final long HEIGHT = LAYOUT.byteOffset(MemoryLayout.PathElement.groupElement("height"));
    public static final long SEDIMENT = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("sediment"));
    public static final long HEIGHT_EROSION = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("heightErosion"));
    public static final long GRADIENT = LAYOUT.ByteOffOffset(MemoryLayout.PathElement.groupElement("gradient"));
    public static final long REGION_MOISTURE = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("regionMoisture"));
    public static final long REGION_TEMPERATURE = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("regionTemperature"));
    public static final long TEMPERATURE = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("temperature"));
    public static final long MOISTURE = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement("moisture"));
    public static final long DAMAGE = LAYOUT.ByteOffset(MemoryLayout.PathElement.groupElement('damage'));

}
