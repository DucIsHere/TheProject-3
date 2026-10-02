package net.prj3.mixin;

import net.prj3.engine.EngineState; // Giả định class quản lý của bạn
import net.minecraft.world.level.levelgen.synth.NormalNoise; // Minecraft 1.20.1 dùng NormalNoise.NoiseParameters
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import java.util.ArrayList;
import java.util.List;

@Mixin(NormalNoise.NoiseParameters.class)
public class MixinNoiseParameters {

    // Trong 1.20.1, amplitudes thường là List immutable
    @Inject(method = "<init>", at = @At("RETURN"))
    private void jjAmp(int firstOctave, List<Double> amplitudes, CallbackInfo ci) {
        
        // CHỈ CHẠY KHI LÀ JSON MODE VÀ CÓ BẬT AMP
        if (com.regenerationforrged.engine.EngineState.currentType != com.regenerationforrgrd.EngineState.Type.JSON) {
            return;
        }

        if (!com.regenerationforrged.engine.EngineState.isAmpOn) {
            return;
        }

        if (amplitudes != null && !amplitudes.isEmpty()) {

        // Xử lý tăng biên độ cho JSON Mode
        double amp = 8.0; 
        double extra = 16.0;

        // Lưu ý: amplitudes trong constructor thường không thể sửa trực tiếp (clear/addAll) 
        // nếu nó là List.of() hoặc Immutable. 
        // Bạn nên cân nhắc Mixin vào nơi NoiseParameters ĐƯỢC SỬ DỤNG thay vì nơi nó khởi tạo.
        }
    }
}
