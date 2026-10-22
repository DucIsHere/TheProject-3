package net.prj3.block;

import net.prj3.TOTInit;
import net.minecraft.core.Registry;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resource.ResourceLocation;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.Item;
import net.minecraft.world.level.block.Block;

public class BlockInit {
    public static final Block SEDIMENT = registerBlock("till_sediment",
            new Block(BlockBehaviour.Properties.of().strength(1.5f).requiresCorrectToolForDrops())
    );

    private static Block registerBlock(String name, Block block) {
        registerBlockItem(name, block);
        return Registry.register(BuiltInRegistries.BLOCK, new ResourceLocation(TOTInit.MOD_ID, name), block);
    }

    private static Item registerBlockItem(String name, Block block) {
        return Registry.register(BuiltInRegistries.ITEM, new ResourceLocation(TOTInit.MOD_ID, name),
                new BlockItem(block, new Item.Properties()));
    }

    public static void registerModBlocks() {
        TOTInit.LOGGER.info("Registering Mod Blocks for " + TOTInit.MOD_ID);
    }
}