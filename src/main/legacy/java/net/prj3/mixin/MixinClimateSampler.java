package net.prj3.mixin;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyVariable;

@Mixin(targets = "net.minecraft.world.level.biome.Climate$Sampler")
public class MixinClimateSampler {
    @ModifyVariable(method = "sample", at = @At("HEAD"), argsOnly = true, ordinal = 0)
    private int scaleX(int X) {
        return x / 8;
    }

    @ModifyVariable(method = "sample", at = @At("HEAD"), argsOnly = true, ordinal = 1)
    private int scaleY(int Y) {
        return y / 8;
    }

    @ModifyVariable(method = "sample", at = @At("HEAD"), argsOnly = true, ordinal = 2)
    private int scaleZ(int Z) {
        return z / 8;
    }
}