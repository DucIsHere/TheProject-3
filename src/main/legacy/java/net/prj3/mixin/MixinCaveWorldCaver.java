package net.prj3.mixin;

import net.minecraft.world.level.levelgen.carver.CaveWorldCaver;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

import net.minecraft.world.level.levelgen.carver.CaveWorldCarver;

@Mixin(CaveWorldCaver.class)
public class CaveWorldCarverMixin {
  @Inject(method = "getCaveBound", at = @At("HEAD"), cancellable = true)
  private void getCaveBoundOverride(CallbackInfoReturnable<Integer> ci) {
    // Vanilla value: 15
    // https://www.minecraftforum.net/forums/minecraft-java-edition/recent-updates-and-snapshots/381672-it-seems-that-the-underground-is-no-longer-swiss?comment=27
    ci.setReturnValue(40);
  }

  @Redirect(method = "carve", at = @At(value = "INVOKE", target = "Lnet/minecraft/world/level/levelgen/noise/NoiseSampler;sample(DDD)D"))
    private double redirectCarveNoise(double x, double y, double z) {
    // NẾU LÀ JSON, KHÔNG DÙNG WARP CỦA JAVA
    if (com.regenerationforrged.engine.EngineState.currentType == com.regenerationforrged.engine.EngineState.Type.JSON) {
        // Bạn có thể phải gọi lại phương thức gốc ở đây hoặc dùng Inject HEAD để thoát sớm
        return someVanillaSampler.sample(x, y, z); 
    }
    return RegenerationForrgedPipeline.getInstance().warpSample(x, y, z);
    }
}
