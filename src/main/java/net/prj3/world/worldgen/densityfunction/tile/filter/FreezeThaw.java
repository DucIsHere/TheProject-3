package net.prj3.world.worldgen.dendityfunction.tile.filter;

import jdk.incubator.foreign.*;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodType;
import java.util.IntFunction;

import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.heightmap.Levels;

public class FreezeThaw implements AutoCloseable{
    private static final SymbolLookup LIBRARIES = SymbolLookup.loaderLookup();
    private static final CLinker LINKER = CLinker.systemLinker();

    private static final MethodHandle CREATE_CONFIG_MH;
    private static final MethodHandle FREE_CONFIG_MH;
    private static final MethodHandle APPLY_FAST_MH;
    private static final MethodHandle INIT_BRUSHES_MH;
    private static final MethodHandle FREE_BRUSHES_MH;

    static {
        System.loadLibrary("FreezeThaw");

        GroupLayout modifierLayout = MemoryLayout.structLayout(
                CLinker.C_FLOAT.withName("min");
                CLinker.C_FLOAT.withName("max");
                CLinker.C_FLOAT.withName("range");
                CLinker.C_FLOAT.withName("is_inverted");
        );

        CREATE_CONFIG_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_create_config").get(),
                MethodHandle.methodType(
                        MemoryAddress.class,
                        int.class,
                        float.class, float.class, float.class, float.class,
                        float.class, float.class, float.class, float.class,
                        int.class, int.class, float.class,
                        MemorySegment.class,
                        float.class, float.class, boolean.class
                ),
                FunctionDescriptor.of(
                        CLinker.C_POINTER,
                        CLinker.C_INT,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_FLOAT,
                        CLinker.C_INT, CLinker.C_INT, CLinker.C_FLOAT,
                        modifierLayout,
                        CLinker.C_FLOAT, CLinker.C_FLOAT, CLinker.C_BOOL
                )
        );

        FREE_CONFIG_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_free_config").get(),
                MethodType.methodType(void.class, MemoryAddress.class),
                FunctionDescriptor.ofVoid(CLinker.C_POINTER)
        );

        APPLY_FAST_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("c23_apply_freeze_thaw_fast").get(),
                MethodType.methodType(
                        void.class,
                        MemoryAddress.class,
                        MemoryAddress.class,
                        MemoryAddress.class,
                        MemoryAddress.class,
                        MemoryAddress.class,
                        int.class, int.class,
                        int.class, int.class,
                        int.class,
                        long.class,
                        int.class,
                        MemoryAddress.class,
                        MemoryAddress.class
                ),
                FunctionDescriptor.ofVoid(
                        CLinker.C_POINTER, CLinker.C_POINTER, CLinker.C_POINTER,
                        CLinker.C_POINTER, CLinker.C_POINTER, CLinker.C_POINTER,
                        CLinker.INT, CLinker.INT, CLinker.INT,
                        CLinker.INT, CLinker.INT, CLinker.LONG,
                        CLinker.INT,
                        CLinker.C_POINTER, CLinker.C_POINTER
                )
        );

        INIT_BRUSHES_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("init_brushes_config").get(),
                MethodType.methodType(MemoryAddress.class, int.class, int.class),
                FunctionDescriptor.of(CLinker.C_POINTER, CLinker.C_INT, CLinker.C_INT)
        );

        FREE_BRUSHES_MH = LINKER.downcallHandle(
                LIBRARIES.lookup("free_idx").get(),
                MethodType.methodType(void.class, MemoryAddress.class),
                FunctionDescriptor.ofVoid(CLinker.C_POINTER)
        );
    }

    private final int mapSize;
    private final int seed;
    private final ResourceScope scope;

    private final MemoryAddress nativeConfig;
    private final MemoryAddress nativeBrushes;

    private final MemorySegment damageMap;
    private final MemorySegment moistureMap;
    private final MemorySegment tempMap;
    private final MemorySegment sedimentMap;

    public FreezeThaw(int mapSize, int seed, Modifier modifier, float porosity, float tensileStrength, float criticalSaturation, float softeningFactor, float clapeyronFactor, float freezeThawCycles, float breakAmount, float zAmountDepth, float dDampingDepth, float modMin, float modMax, boolean modInverted) {
        this.mapSize = mapSize;
        this.seed = seed;
        this.scope = ResourceScope.newSharedScope();

        int totalCells = mapSize * mapSize;
        long byteSize = (long) totalCells * CLinker.C_FLOAT.byteSize();

        this.damageMap   = MemorySegment.allocateNative(byteSize, 4, scope);
        this.moistureMap = MemorySegment.allocateNative(byteSize, 4, scope);
        this.tempMap     = MemorySegment.allocateNative(byteSize, 4, scope);
        this.sedimentMap = MemorySegment.allocateNative(byteSize, 4, scope);

        try {
            MemorySegment dumyModifier = MemorySegment.allocateNative(16, scope);

            this.nativeConfig = (MemoryAddress) CREATE_CONFIG_MH.invoke(
                    mapSize,
                    porosity, tensileStrength, criticalSaturation, softeningFactor,
                    freezeThawCycles, breakAmount, zAmountDepth, dDampingDepth,
                    0, 0, 1.0f,
                    dumyModifier,
                    modMin, modMax, modInverted
            );

            this.nativeBrushes = (MemoryAddress) INIT_BRUSHES_MH.invoke(mapSize, 4);
        } catch (Throwable e) {
            throw new RuntimeException(e);
        }
    }

    public int getSize() {
        return this.mapSize;
    }

    @Override
    public void apply(Filterable map, int regionX, int regionZ, int iterations) {
        final Size size = map.getBlockSize();
        final int width = size.width();
        final int height = size.height();
        final int border = size.border();
        final int blockX = map.getBlockX();
        final int blockZ = map.getBlockZ();

        MemoryAddress cellsAddress = MemoryAddress.ofLong(map.getBackingAddress());

        MemoryAddress threadPoolAddress = MemoryAddress.NULL;

        try {
            APPLY_FAST_MH.invoke(
                    threadPoolAddress,
                    this.nativeConfig,
                    cellsAddress,
                    this.tempMap.address(),     // Scratch Damage
                    this.sedimentMap.address(), // Scratch Moisture
                    blockX, blockZ,
                    width, height,
                    border,
                    (long) this.seed,
                    iterations,
                    this.damageMap.address(),
                    this.moistureMap.address()
            );
        } catch (Throwable e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void close() {
        try {
            if (nativeBrushes != null && !nativeBrushes.equals(MemoryAddress.NULL)) {
                FREE_BRUSHES_MH.invoke(nativeBrushes);
            }
            if (nativeConfig != null && !nativeConfig.equals(MemoryAddress.NULL)) {
                FREE_CONFIG_MH.invoke(nativeConfig);
            }
        } catch (Throwable e) {
            e.printStackTrace();
        } finally {
            if (scope.isAlive()) {
                scope.close();
            }
        }
    }

    public static class Factory implements IntFunction<FreezeThaw> {
        private static final int SEED_OFFSET = 13578;
        private final int seed;
        private final Modifier modifier;
        private final FilterSettings.FreezeThaw settings;
        private final float modMin;
        private final float modMax;

        public Factory(final int seed, final FilterSettings filters, final Levels levels) {
            this.seed = seed + SEED_OFFSET;
            this.settings = filters.freezeThaw.copy();
            this.modMin = levels.ground;
            this.modMax = levels.ground(15);
            this.modifier = Modifier.range(this.modMin, this.modMax);
        }

        @Override
        public FreezeThaw apply(final int size) {
            return new FreezeThaw(
                    size,
                    this.seed,
                    this.modifier,
                    this.settings.porosity,
                    this.settings.tensileStrength,
                    this.settings.criticalSaturation,
                    this.settings.softeningFactor,
                    1.11F,
                    this.settings.freezeThawCycles,
                    this.settings.breakAmount,
                    this.settings.zAmountDepth,
                    this.settings.dDampingDepth,
                    this.modMin,
                    this.modMax,
                    false
            );
        }
    }
}