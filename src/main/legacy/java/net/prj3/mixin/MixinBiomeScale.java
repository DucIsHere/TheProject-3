package net.prj3.mixin;

import net.minecraft.world.level.StructureManager;
import net.minecraft.world.level.WorldGenLevel;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.ChunkGenerator;
import net.minecraft.world.level.biome.Climate;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injecttion.At;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

public class MixinBiomeScale {
    @Unique
    private static final ThreadLocal<Boolean> IS_PLACING_FEATURE = ThreadLocal.withInitial(() -> false);

    @Unique
    private static final float STRETCH_FACTOR = 0.083333f;

    @Mixin(ChunkGenerator.class)
    public static class FeaturePhase {
        @Inject (method = "applyBiomeDecoration", at = @At("HEAD"))
        private void onStart(WorldGenLevel level, ChunkAccess chunk, StructureManager structureManager, CallbackInfo ci) {
            IS_PLACING_FEATURE.set(true);
        }

        @Inject (method = "applyBiomeDecoration", at = @At("TAIL")) 
        private void onEnd(WorldGenLevel level, ChunkAccess chunk, StructureManager structureManager, CallbackInfo ci) {
            IS_PLACING_FEATURE.set(false);
        }
    }

    @Mixin(Climate.Sampler.class)
    public static class SamplerSpoofer {
        @Inject (method = "sample", at = @At("HEAD"), cancellable = true)
        private void spoofCoordinates(int x, int y, int z, CallbackInfoReturnable<Climate.TargetPoint> cir) {
            if (IS_PLACING_FEATURES.get()) {
                return;
            }
            int spoofedX = (int) Math.floor(x * STRETCH_FACTOR);
            int spoofedZ = (int) Math.floor(z * STRETCH_FACTOR);
        }
    }
}
