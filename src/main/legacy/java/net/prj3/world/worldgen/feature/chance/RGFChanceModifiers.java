package net.prj3.world.worldgen.feature.chance;

import com.mojang.serialization.Codec;

import net.prj3.platform.RegistryUtil;
import net.prj3.registries.RTFBuiltInRegistries;

public class RGFChanceModifiers {

	public static void bootstrap() {
		register("elevation", ElevationChanceModifier.CODEC);
		register("biome_edge", BiomeEdgeChanceModifier.CODEC);
	}
	
	public static ElevationChanceModifier elevation(float from, float to) {
		return elevation(from, to, false);
	}
	
	public static ElevationChanceModifier elevation(float from, float to, boolean exclusive) {
		return new ElevationChanceModifier(from, to, exclusive);
	}
	
	public static BiomeEdgeChanceModifier biomeEdge(float from, float to) {
		return biomeEdge(from, to, false);
	}
	
	public static BiomeEdgeChanceModifier biomeEdge(float from, float to, boolean exclusive) {
		return new BiomeEdgeChanceModifier(from, to, exclusive);
	}
	
	private static void register(String name, Codec<? extends ChanceModifier> placement) {
		RegistryUtil.register(RTFBuiltInRegistries.CHANCE_MODIFIER_TYPE, name, placement);
	}
}
