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
    }
}