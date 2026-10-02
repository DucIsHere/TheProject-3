package net.prj3.world.worldgen;

import org.jetbrains.annotations.Nullable;

import net.minecraft.core.RegistryAccess;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.world.worldgen.noise.module.Noise;

public interface RGFRandomState {
	void initialize(RegistryAccess registries);
	
	@Nullable
	Preset preset();

	@Nullable
	GeneratorContext generatorContext();
	
	DensityFunction wrap(DensityFunction function);

	Noise seed(Noise noise);
}
