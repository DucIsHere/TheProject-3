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
            ValueLayout.JAVA_FLOAT.withName("temperature"),
            ValueLayout.JAVA_FLOAT.withName("moisture"),
            ValueLayout.JAVA_FLOAT.withName("damage"),
            ValueByteLayout.JAVA_BYTE.withName("erosionMask")
    );

    public static final VarHandle HEIGHT = LAYOUT.varHandle(PathElement.groupElement("height"));
    public static final VarHandle SEDIMENT = LAYOUT.varHandle(PathElement.groupElement("sediment"));
    public static final VarHandle HEIGHT_EROSION = LAYOUT.varHandle(PathElement.groupElement("heightErosion"));
    public static final VarHandle GRADIENT = LAYOUT.varHandle(PathElement.groupElement("gradient"));
}
