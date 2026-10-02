package net.prj3.worldgen.world.cell.terrain;

import net.prj3.data.worldgen.preset.settings.TerrianSettings;

public class TerrianComposer {

    public double sample(int x, int y, int z, TerrianSettings settings, TerrianType type) {
        // call ITerrian
        return type.impl.sample(x, y, z, settings);
    }
}
