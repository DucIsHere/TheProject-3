package net.prj3.world.worldgen.biome.modifier;

import java.util.function.Function;

import com.mojang.serialization.Codec;

import net.prj3.registries.RGFBuiltInRegistries;

// theres other worldgen libraries we can use for this that aren't so janky
@Deprecated(forRemoval = true)
public interface BiomeModifier {
    public static final Codec<BiomeModifier> CODEC = RGFBuiltInRegistries.BIOME_MODIFIER_TYPE.byNameCodec().dispatch(BiomeModifier::codec, Function.identity());
	
	Codec<? extends BiomeModifier> codec();
}
