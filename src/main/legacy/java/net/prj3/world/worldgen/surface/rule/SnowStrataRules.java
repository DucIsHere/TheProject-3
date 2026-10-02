package net.prj3.world.worldgen.surface.rule;

import com.mojang.serialization.Codec;
import com.mojang.serialization.codecs.RecordCodecBuilder;
import net.minecraft.core.Holder;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.SnowLayerBlock;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.levelgen.SurfaceRules;
import net.minecraft.util.KeyDispatchDataCodec;
import net.prj3.world.worldgen.noise.module.Noise;
import net.prj3.world.worldgen.RTFRandomState;

public record SnowStrataRules(Holder<Noise> depthNoise, Holder<Noise> driftNoise) implements SurfaceRules.RuleSource {
    public static final Codec<SnowStrataRules> CODEC = RecordCodecBuilder.create(instance -> instance.group(
        Noise.CODEC.fieldOf("depth_noise").forGetter(SnowStrataRules::depthNoise),
        Noise.CODEC.fieldOf("drift_noise").forGetter(SnowStrataRules::driftNoise)
    ).apply(instance, SnowStrataRules::new));

    @Override
    public SurfaceRules.SurfaceRule apply(SurfaceRules.Context ctx) {
        if ((Object) ctx.randomState instanceof RTFRandomState rtfState) {
            return new State(this, ctx, rtfState);
        }
        return null;
    }

    @Override
    public KeyDispatchDataCodec<? extends SurfaceRules.RuleSource> codec() {
        return KeyDispatchDataCodec.of(CODEC);
    }

    static class State implements SurfaceRules.SurfaceRule {
        private final SnowStrataRules config;
        private final SurfaceRules.Context context;
        private final RTFRandomState rtfState;
        
        private int lastXZ = -1;
        private int surfaceY;
        private int snowDepth;
        private int snowLayers;

        State(AdvancedSnowStrataRule config, SurfaceRules.Context context, RTFRandomState rtfState) {
            this.config = config;
            this.context = context;
            this.rtfState = rtfState;
        }

        @Override
        public BlockState tryApply(int x, int y, int z) {
            if (this.lastXZ != this.context.lastUpdateXZ) {
                this.update(x, z);
                this.lastXZ = this.context.lastUpdateXZ;
            }

            // Nếu độ dốc quá cao (> 45 độ), tuyết không đọng lại
            if (this.snowDepth <= 0) return null;

            // Lớp phủ mặt (Snow Layer với độ dày biến thiên)
            if (y == this.surfaceY) {
                return Blocks.SNOW.defaultBlockState().setValue(SnowLayerBlock.LAYERS, this.snowLayers);
            }

            // Lớp lõi (Snow Block)
            if (y < this.surfaceY && y >= (this.surfaceY - this.snowDepth)) {
                return Blocks.SNOW_BLOCK.defaultBlockState();
            }

            return null;
        }

        private void update(int x, int z) {
            this.surfaceY = this.context.chunk.getHeight(Heightmap.Types.WORLD_SURFACE_WG, x & 15, z & 15);
            
            // 1. Tính toán độ dốc (Steepness) - Tuyết tỷ lệ nghịch với độ dốc
            // Giả sử rtfState có thể truy xuất Steepness từ Cell hoặc Noise
            float steepness = rtfState.getSteepness(x, z); 
            float slopeFactor = Math.max(0, 1.0f - (steepness * 1.5f)); 

            // 2. Logic tích tụ theo độ cao (Exponential Accumulation)
            // Càng lên cao tuyết càng dày nhanh hơn thay vì tuyến tính
            float heightAlpha = Math.max(0, (float)(this.surfaceY - 80) / 400.0f);
            float baseThickness = (float) Math.pow(heightAlpha, 1.5) * 12.0f;

            // 3. Noise Drifts (Gió thổi tạo vỉa)
            float drift = this.config.driftNoise().value().compute(x * 0.05, z * 0.05, 0) * 4.0f;
            float depthRaw = (baseThickness + drift) * slopeFactor;

            this.snowDepth = Math.round(depthRaw);
            
            // 4. Tính toán Layers (1-8) cho block trên cùng để mượt địa hình
            float fractional = depthRaw - (int)depthRaw;
            this.snowLayers = Math.max(1, Math.min(8, Math.round(fractional * 8)));
            
            // Nếu dốc quá thì xóa luôn tuyết
            if (steepness > 0.6f) this.snowDepth = -1;
        }
    }
}