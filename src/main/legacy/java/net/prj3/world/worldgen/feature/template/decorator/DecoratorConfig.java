package net.prj3.world.worldgen.feature.template.decorator;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import java.util.List;

public class DecoratorConfig<T> {
    public final List<TemplateDecorator<T>> decorators;

    public static final Codec<DecoratorConfig> CODEC = RecordCodecBuilder.create(inst -> inst.group(
        // placeholder: decorators require custom codec per decorator type
        Codec.list(Codec.unit(null)).fieldOf("decorators").forGetter(d -> List.of())
    ).apply(inst, v -> new DecoratorConfig<>(List.of())));

    public DecoratorConfig(List<TemplateDecorator<T>> decorators) {
        this.decorators = decorators;
    }
}
