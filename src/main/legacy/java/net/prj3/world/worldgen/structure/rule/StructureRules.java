package net.prj3.world.worldgen.structure.rule;

import com.google.common.collect.ImmutableSet;
import com.mojang.serialization.Codec;

import net.prj3.platform.RegistryUtil;
import net.prj3.registries.RGFBuiltInRegistries;
import net.prj3.world.worldgen.cell.terrain.Terrain;

public class StructureRules {

	public static void bootstrap() {
		register("cell_test", CellTest.CODEC);
	}
	
	public static CellTest cellTest(float cutoff, Terrain... terrainTypeBlacklist) {
		return new CellTest(cutoff, ImmutableSet.copyOf(terrainTypeBlacklist));
	}

	private static void register(String name, Codec<? extends StructureRule> value) {
		RegistryUtil.register(RGFBuiltInRegistries.STRUCTURE_RULE_TYPE, name, value);
	}
}
