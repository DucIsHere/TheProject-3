package net.prj3.data.worldgen;

import net.minecraft.data.worldgen.BootstapContext;
import net.minecraft.resources.ResourceKey;
import net.prj3.RegenerationForrged;
import net.prj3.data.worldgen.preset.Preset;
import net.prj3.registries.RTFRegistries;
import net.prj3.world.worldgen.cell.terrain.TerrainType;
import net.prj3.world.worldgen.structure.rule.StructureRule;
import net.prj3.world.worldgen.structure.rule.StructureRules;

public class StructureRuleData {
	public static final ResourceKey<StructureRule> CELL_TEST = createKey("cell_test");
	
	public static void bootstrap(Preset preset, BootstapContext<StructureRule> ctx) {
		ctx.register(CELL_TEST, StructureRules.cellTest(0.125F, TerrainType.MOUNTAIN_CHAIN, TerrainType.MOUNTAINS_1, TerrainType.MOUNTAINS_2, TerrainType.MOUNTAINS_3));
	}
	
	private static ResourceKey<StructureRule> createKey(String name) {
        return ResourceKey.create(RTFRegistries.STRUCTURE_RULE, RTFCommon.location(name));
	}
}
