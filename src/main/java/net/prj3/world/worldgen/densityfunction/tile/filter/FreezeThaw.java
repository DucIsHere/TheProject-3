package net.prj3.world.worldgen.dendityfunction.tile.filter;

import jdk.incubator.foreign.*;
import java.lang.invoke.MethodHandles;
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
    }
}