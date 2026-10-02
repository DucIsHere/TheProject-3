package net.prj3.mixin;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.biome.Climate;
import net.minecraft.world.level.levelgen.RandomState;
import net.minecraft.world.level.storage.ServerLevelData;
import net.prj3.data.worldgen.preset.settings.Preset;
import net.prj3.registries.RGFRegistries;

@Mixin(MinecraftServer.class)
class MixinMinecraftServer {

	@Inject(at = @At("HEAD"), method = "setInitialSpawn")
    private static void findSpawnPosition(ServerLevel serverLevel, ServerLevelData serverLevelData, boolean bl, boolean bl2, CallbackInfo callback) {
    serverLevel.registryAccess().lookup(RGFRegistries.PRESET).flatMap(r -> r.get(Preset.KEY)).ifPresent(preset -> {
		boolean isJava = preset.value().world().properties.isJavaEngine;
        // Kiểm tra ID của Preset để chốt Mode
        if (isJava) {
        com.regenerationforrged.engine.EngineState.currentType = Type.JAVA;
        // Cho phép hệ thống load TerrainSettings, RiverSettings...
		} else {
        com.regenerationforrged.engine.EngineState.currentType = Type.JSON;
        // Dòng này sẽ kích hoạt "lệnh ngắt" return trong 17+ Mixins của bạn
		}
    });
}
}
