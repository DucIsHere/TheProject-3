package net.prj3.data.worldgen.preset;

import com.mojang.serialization.Codec;

import net.minecraft.util.StringRepresentable;
import net.prj3.world.worldgen.GeneratorContext;
import net.prj3.world.worldgen.cell.continent.Continent;
import net.prj3.world.worldgen.cell.continent.advanced.AdvancedContinentGenerator;
import net.prj3.world.worldgen.cell.continent.fancy.FancyContinentGenerator;
import net.prj3.world.worldgen.cell.continent.infinite.InfiniteContinentGenerator;
import net.prj3.world.worldgen.cell.continent.simple.MultiContinentGenerator;
import net.prj3.world.worldgen.cell.continent.simple.SingleContinentGenerator;
import net.prj3.world.worldgen.util.Seed;

public enum ContinentType implements StringRepresentable {
    MULTI {
        
    	@Override
        public MultiContinentGenerator create(Seed seed, GeneratorContext context) {
            return new MultiContinentGenerator(seed, context);
        }
    }, 
    SINGLE {
        
    	@Override
        public SingleContinentGenerator create(Seed seed, GeneratorContext context) {
            return new SingleContinentGenerator(seed, context);
        }
    }, 
    MULTI_IMPROVED {
        
    	@Override
        public AdvancedContinentGenerator create(Seed seed, GeneratorContext context) {
            return new AdvancedContinentGenerator(seed, context);
        }
    }, 
    EXPERIMENTAL {
        
    	@Override
        public FancyContinentGenerator create(Seed seed, GeneratorContext context) {
            return new FancyContinentGenerator(seed, context);
        }
    },
    INFINITE {
        
    	@Override
        public InfiniteContinentGenerator create(Seed seed, GeneratorContext context) {
            return new InfiniteContinentGenerator(context);
        }
    };
	
	public static final Codec<ContinentType> CODEC = StringRepresentable.fromEnum(ContinentType::values);
    
    public abstract Continent create(Seed seed, GeneratorContext context);

    @Override
	public String getSerializedName() {
		return this.name();
	}
}
