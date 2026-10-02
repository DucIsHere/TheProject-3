package net.prj3.world.worldgen.feature.template.decorator;

import net.minecraft.world.level.levelgen.feature.FeaturePlaceContext;

public interface TemplateDecorator<T> {
    boolean apply(T context);
}
