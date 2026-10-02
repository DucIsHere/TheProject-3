package net.prj3;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import net.minecraft.resources.ResourceLocation;
import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.platform.RegistryUtil;
import net.prj3.registries.RGFBuiltInRegistries;
import net.prj3.registries.RGFRegistries;
import net.prj3.world.worldgen.biome.modifier.BiomeModifiers;
import net.prj3.world.worldgen.densityfunction.RHFDensityFunctions;
import net.prj3.world.worldgen.feature.RHFFeatures;
import net.prj3.world.worldgen.feature.chance.RGFChanceModifiers;
import net.prj3.world.worldgen.feature.placement.RGFPlacementModifiers;
import net.prj3.world.worldgen.feature.template.decorator.TemplateDecorators;
import net.prj3.world.worldgen.feature.template.placement.TemplatePlacements;
import net.prj3.world.worldgen.floatproviders.RGFFloatProviderTypes;
import net.prj3.world.worldgen.heightproviders.RHFHeightProviderTypes;
import net.prj3.world.worldgen.noise.domain.Domains;
import net.prj3.world.worldgen.noise.function.CurveFunctions;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.noise.module.Noises;
import net.prj3.world.worldgen.structure.rule.StructureRule;
import net.prj3.world.worldgen.structure.rule.StructureRules;
import net.prj3.world.worldgen.surface.rule.RGFSurfaceRules;

public class TOTInit {
	public static final String MOD_ID = "regenerationforrgrd";
	public static final String LEGACY_MOD_ID = "terraforged";
	public static final String ORIGINAL_MO_ID = "reterraforged";
	public static final Logger LOGGER = LogManager.getLogger("TerrianOfTerrian");

	public static void bootstrap() {
		RGFBuiltInRegistries.bootstrap();
		TemplatePlacements.bootstrap();
		TemplateDecorators.bootstrap();
		RGFChanceModifiers.bootstrap();
		RGFPlacementModifiers.bootstrap();
		RGFDensityFunctions.bootstrap();
		Noises.bootstrap();
		Domains.bootstrap();
		CurveFunctions.bootstrap();
		RGFFeatures.bootstrap();
		RGFHeightProviderTypes.bootstrap();
		RGFFloatProviderTypes.bootstrap();
		BiomeModifiers.bootstrap();
		RGFSurfaceRules.bootstrap();
		StructureRules.bootstrap();
		
		RegistryUtil.createDataRegistry(RGFRegistries.NOISE, Noise.DIRECT_CODEC);
		RegistryUtil.createDataRegistry(RGFRegistries.PRESET, Preset.DIRECT_CODEC);
		RegistryUtil.createDataRegistry(RGFRegistries.STRUCTURE_RULE, StructureRule.DIRECT_CODEC);
	}
	
	public static ResourceLocation location(String name) {
		if (name.contains(":")) return new ResourceLocation(name);
		return new ResourceLocation(TOTInit.MOD_ID, name);
	}
}
