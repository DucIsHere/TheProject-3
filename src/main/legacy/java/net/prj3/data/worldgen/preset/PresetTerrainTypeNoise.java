package net.prj3.data.worldgen.preset;

import net.minecraft.data.worldgen.BootstapContext;
import net.minecraft.resources.ResourceKey;

import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.data.worldgen.preset.settings.WorldSettings;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.noise.module.Noises;

public class PresetTerrainTypeNoise {
	public static final ResourceKey<Noise> GROUND = PresetTerrainNoise.createKey("ground");
	
	public static void bootstrap(Preset preset, BootstapContext<Noise> ctx) {
		WorldSettings worldSettings = preset.world();
		WorldSettings.Properties properties = worldSettings.properties;
		float seaLevel = properties.seaLevel;
		int terrainScaler = properties.terrainScaler();

		ctx.register(GROUND, Noises.constant(seaLevel / (float)terrainScaler));
	}
}