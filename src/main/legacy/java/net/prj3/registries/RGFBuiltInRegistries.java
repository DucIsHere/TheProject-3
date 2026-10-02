package net.prj3.registries;

import com.mojang.serialization.Codec;

import net.minecraft.core.Registry;
import net.prj3.platform.RegistryUtil;
import net.prj3.world.worldgen.biome.modifier.BiomeModifier;
import net.prj3.world.worldgen.feature.chance.ChanceModifier;
import net.prj3.world.worldgen.feature.template.decorator.TemplateDecorator;
import net.prj3.world.worldgen.feature.template.placement.TemplatePlacement;
import net.prj3.world.worldgen.noise.domain.Domain;
import net.prj3.world.worldgen.noise.function.CurveFunction;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.structure.rule.StructureRule;

public class RTFBuiltInRegistries {
	public static final Registry<Codec<? extends Noise>> NOISE_TYPE = RegistryUtil.createRegistry(RTFRegistries.NOISE_TYPE);
	public static final Registry<Codec<? extends Domain>> DOMAIN_TYPE = RegistryUtil.createRegistry(RTFRegistries.DOMAIN_TYPE);
	public static final Registry<Codec<? extends CurveFunction>> CURVE_FUNCTION_TYPE = RegistryUtil.createRegistry(RTFRegistries.CURVE_FUNCTION_TYPE);
	public static final Registry<Codec<? extends ChanceModifier>> CHANCE_MODIFIER_TYPE = RegistryUtil.createRegistry(RTFRegistries.CHANCE_MODIFIER_TYPE);
	public static final Registry<Codec<? extends TemplatePlacement<?>>> TEMPLATE_PLACEMENT_TYPE = RegistryUtil.createRegistry(RTFRegistries.TEMPLATE_PLACEMENT_TYPE);
	public static final Registry<Codec<? extends TemplateDecorator<?>>> TEMPLATE_DECORATOR_TYPE = RegistryUtil.createRegistry(RTFRegistries.TEMPLATE_DECORATOR_TYPE);
	public static final Registry<Codec<? extends BiomeModifier>> BIOME_MODIFIER_TYPE = RegistryUtil.createRegistry(RTFRegistries.BIOME_MODIFIER_TYPE);
	public static final Registry<Codec<? extends StructureRule>> STRUCTURE_RULE_TYPE = RegistryUtil.createRegistry(RTFRegistries.STRUCTURE_RULE_TYPE);

	public static void bootstrap() {
	}
}
