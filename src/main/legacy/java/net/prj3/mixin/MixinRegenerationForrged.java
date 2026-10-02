package net.prj3.mixin;

import net.minecraft.server.MinecraftServer;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.levelgen.Aquifer;
import net.minecraft.world.level.levelgen.NoiseGeneratorSettings;
import net.minecraft.world.level.levelgen.NoiseBasedChunkGenerator;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallBackInfoReturnable;

@Mixin(NoiseChunkGenerator.class)
public class MixinRegenerationForrged {
    @Inject(method = "createFluidPicker", at = @At("HEAD"), cancellable = true)
    private static void createFluidPicker(NoiseGeneratorSettings settings,
            CallbackInfoReturnable<Aquifer.FluidPicker> cir) {

        int lavaSea = settings.noiseSettings().minY() + 10;
        var lavaStatus = new Aquifer.FluidStatus(lavaSea, Blocks.LAVA.defaultBlockState());

        int sea = settings.seaLevel();
        var defaultStatus = new Aquifer.FluidStatus(sea, settings.defaultFluid());

        cir.setReturnValue((x, y, z) -> y < Math.min(lavaSea, sea) ? lavaStatus : defaultStatus);
    }
}
