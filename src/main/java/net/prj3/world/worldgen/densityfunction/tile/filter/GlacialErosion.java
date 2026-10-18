package net.prj3.world.worldgen.densityfunction.tile.filter;

import jdk.incubator.foreign.*;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.util.IntFunction;

import net.prj3.data.worldgen.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.densityfunction.tile.Size;

public class GlacialErosion extends Filter{
    private static final SymbolLookup LIBRARIES = SymbolLookup.loaderLookup();
    private static final CLinker LINKER = CLinker.systemLinker();

    private static final MethodHanlde CREATE_CONFIG_MH;
    private static final MethodHandle FREE_CONFIG_MH;
    private static final MethodHandle APPLY_GLACIAL;
    private static final MethodHandle INIT_BRUSHES;
    private static final MethodHandle FREE_BRUSHES;

    static {
        System.loadLibrary("GlacialErosion");

        GroupLayout modifierLayout = MemoryLayout.structLayout(
                CLinker.C_FLOAT.withName("min");
                CLinker.C_FLOAT.withName("max");
                CLinker.C_FLOAT.withName("range");
                CLinker.C_FLOAT.withName("is_inverted");
        );

        CREATE_CONFIG_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_glacial_conf").get();
                MethodType.handleType(
                        MemoryAddress.class,
                        int.class, int.class,
                        MemoryAddress.class,
                        float.class, float.class, float.class, float.class, float.class,
                        float.class, float.class, float.class, float.class, float.class,
                        float.class, float.class, float.class, float.class, float.class,
                        float.class, float.class,
                        int.class, int.class, int.class,
                        float.class,
                        int.class
                ),
                FunctionDescriptor.of(
                        CLinker.C_POINTER,
                        CLinker.C_INT, CLinker.C_INT,
                        CLinker.C_POINTER,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT,
                        CLinker.C_FLOAT, CLinker.C_INT,
                        CLinker.C_INT, CLinker.C_INT, CLinker.C_INT,
                        CLinker.C_FLOAT,
                        CLinker.C_INT
                ),
        );

        FREE_CONFIG_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_glacial_free"),
                MethodType.handleType(void.class, MemoryAddress.class),
                FunctionDescriptor.ofVoid(CLinker.C_POINTER)
        );

        APPLY_GLACIAL = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_run_glacial_simulation");
                MethodType.handleType(
                        MemoryAddress.class,
                        MemoryAddress.class,
                        MemoryAddress.class,
                        int.class, int.class, int.class,
                        int.class, int.class, int.class,
                        long.class,
                        int.class
                ),
                FunctionDescriptor.of(
                        CLinker.C_POINTER,
                        CLinker.C_POINTER,
                        CLinker.C_POINTER,
                        CLinker.C_INT, CLinker.C_INT, CLinker.C_INT,
                        CLinker.C_INT, CLinker.C_INT, CLinker.C_INT,
                        CLinker.C_LONG,
                        CLinker.C_INT
                )
        );

        INIT_BRUSHES = LINKER.downcall(
                LIBRARIES.lookup("init_brushes_config");
                MethodType.handleType(MemoryAdress.class, int.class, int.class),
                FunctionDescriptor.of(CLinker.C_POINTER, CLinker.C_INT, CLinker.C_INT)
        );

        FREE_BRUSHES = LINKER.downcall(
                LIBRARIES.lookup("free_idx");
                MethodType.handleType(void.class, MemoryAddress.class),
                FunctionDescriptor.ofVoid(CLinker.C_POINTER)
        );
    }

    private final int mapSize;
    private final int seed;
    private final ResourceScope scope;

    private final MemoryAddress nativeConfig;
    private final MemoryAddress nativeBrushes;

    private final MemorySegment iceThickness;
    private final MemorySegment iceVelocity;
    private final MemorySegment basalStress;
    private final MemorySegment surfaceMassBalance;
    private final MemorySegment cumulativeDamage;
    private final MemorySegment bedrockDamage;
    private final MemorySegment subglacialTill;

    public GlacialErosion(int mapSize, int seed, float slidingCoefficient, float erosionConstant, float viscosityFactor, float kinWaveSpees, float elaHeight, float massBalanceGradient, float flexuralRigidity, float thermalDiffusivity, float degreeDayFactor, float tillTransportRate, float effectivePressureCoeff, float cryoDamageRate) {
        this.mapSize = mapSize;
        this.seed = seed;
        this.scope = ResourceScope.newSharedScope();

        int totalCells = mapSize * mapSize;
        long byteSize = (long) totalCells * CLinker.C_FLOAT.byteSize();
    }
}