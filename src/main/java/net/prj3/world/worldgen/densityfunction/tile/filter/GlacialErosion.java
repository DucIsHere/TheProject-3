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

        this.iceThickness = MemorySegment.allocateNative(byteSize, 4, scope);
        this.iceVelocity = MemorySegment.allocateNative(byteSize, 4, scope);
        this.basalStress = MemorySegment.allocateNative(byteSize, 4, scope);
        this.surfaceMassBalance = MemorySegment.allocateNative(byteSize, 4, scope);
        this.cumulativeDamage = MemorySegment.allocateNative(byteSize, 4, scope);
        this.bedrockDamage = MemorySegment.allocateNative(byteSize, 4, scope);
        this.subglacialTill = MemorySegment.allocateNative(byteSize, 4, scope);

        try {
            MemorySegment dumyModifier = MemorySegment.allocateNative(28, scope);

            this.nativeConfig = (MemoryAddress) CREATE_CONFIG_MH.invoke(
                    mapSize,
                    slidingCoefficient, erosionConstant, thermalDiffusivity, viscosityFactor,
                    effectivePressureCoeff, elaHeight, kinWaveSpees, degreeDayFactor, massBalanceGradient,
                    flexuralRigidity, cryoDamageRate,
                    0, 0, 1.0f,
                    dumyModifier,
                    modMin, modMax, modInverted
            );

            this.nativeBrushes = (MemoryAddress) INIT_BRUSHES.invoke(mapSize, 28);
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
        final int chunkX = map.getBlockX();
        final int chunkZ = map.getBlockZ();
        final int lengthChunk = map.getBlockSize().total() << 4;
        final int borderChunk = map.getBlockSize().total() >> 4;

        MemoryAddress cellsAddress = MemoryAddress.ofLong(map.getBackingAddress());

        MemoryAddress threadPoolAddress = MemoryAddress.NULL;

        try {
            APPLY_GLACIAL.invoke(
                    threadPoolAddress,
                    this.nativeConfig,
                    cellsAddress,
                    this.iceThickness.address();
                    this.basalStress.address();
                    this.surfaceMassBalance.address();
                    this.cumulativeDamage.address();
                    this.bedrockDamage.address();
                    this.subglacialTill.address();
                    this.iceVelocity.address();
                    chunkX, chunkZ,
                    lengthChunk, borderChunk, (long) this.seed,
                    iterations
            );
        } catch (Throwable e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void close() {
        try {
            if (nativeBrushes != null && !nativeBrushes.equals(MemoryAddress.NULL)) {
                FREE_BRUSHES.invoke(nativeBrushes);
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

    public static class Factory implements IntFunction<GlacialErosion> {
        private static final int SEED_OFFSET = 123789;
        private final int seed;
        private final Modifier modifier;
        private final FilterSettings.GlacialErosion settings;
        private final float modMin;
        private final float modMax;

        private Factory(final int seed, final FilterSettings filters, float Levels levels) {
            this.seed = seed;
            this.settings = filters.GlacialErosion.copy();
            this.modMin = levels.ground;
            this.modMax = levels.ground(15);
            this.modifier = modifier.range(this.modMin, this.modMax);

            @Override
            public GlacialErosion apply(final int size) {
                return new GlacialErosion(
                        size,
                        this.seed,
                        this.modifier,
                        this.settings.slidingCoefficient,
                        this.settings.erosionConstant,
                        this.settings.viscosityFactor,
                        this.settings.kinWaveSpeed,
                        this.settings.elaHeight,
                        this.settings.massBalanceGradient,
                        this.settings.flexuralRigidity,
                        this.settings.thermalDiffusivity,
                        this.settings.degreeDayFactor,
                        this.settings.tillTransportRate,
                        this.settings.effectivePressureCoeff,
                        this.settings.cryoDamageRate,
                        this.modMin,
                        this.modMax,
                        false
                );
            }
        }
    }
}