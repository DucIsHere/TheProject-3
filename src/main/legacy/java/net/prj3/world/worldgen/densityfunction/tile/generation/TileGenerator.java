package net.prj3.world.worldgen.densityfunction.tile.generation;

import java.util.concurrent.CompletableFuture;

import net.prj3.concurrent.ThreadPools;
import net.prj3.concurrent.pool.ArrayPool;
import net.prj3.world.worldgen.WorldFilters;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.heightmap.Heightmap;
import net.prj3.world.worldgen.cell.rivermap.Rivermap;
import net.prj3.world.worldgen.densityfunction.tile.Size;
import net.prj3.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.densityfunction.tile.Tile.Chunk;

public class TileGenerator {
	private Heightmap heightmap;
	private WorldFilters filters;
	private ArrayPool<Cell> cellPool;
	private ArrayPool<Chunk> chunkPool;
	private int tileChunks;
	private int tileBorder;
	private Size tileSizeBlocks;
	private Size tileSizeChunks;
	private int batchSize;
	private int batchCount;
	
	public TileGenerator(Heightmap heightmap, WorldFilters filters, int tileChunks, int tileBorder, int batchCount) {
		this.heightmap = heightmap;
		this.filters = filters;
		this.cellPool = ArrayPool.of(100, (length) -> {
			Cell[] cells = new Cell[length];
			for(int i = 0; i < cells.length; i++) {
				cells[i] = new Cell();
			}
			return cells;
		});
		this.chunkPool = ArrayPool.of(100, Chunk[]::new);
		this.tileChunks = tileChunks;
		this.tileBorder = tileBorder;
		this.tileSizeBlocks = Size.blocks(tileChunks, tileBorder);
		this.tileSizeChunks = Size.chunks(tileChunks, tileBorder);
		this.batchSize = getBatchSize(batchCount, this.tileSizeChunks);
		this.batchCount = batchCount;
	}
	
	public Heightmap getHeightmap() {
		return this.heightmap;
	}
	
	public CompletableFuture<Tile> generate(int tileX, int tileZ) {
		Tile tile = this.makeTile(tileX, tileZ);
		CompletableFuture<?>[] futures = new CompletableFuture<?>[this.batchCount * this.batchCount];
		for (int batchZ = 0; batchZ < this.batchCount; batchZ++) {
			for (int batchX = 0; batchX < this.batchCount; batchX++) {
				int chunkX = batchX * this.batchSize;
				int chunkZ = batchZ * this.batchSize;
				futures[batchX * this.batchCount + batchZ] = CompletableFuture.runAsync(() -> {
			        int maxX = Math.min(this.tileSizeChunks.total(), chunkX + this.batchSize);
			        int maxZ = Math.min(this.tileSizeChunks.total(), chunkZ + this.batchSize);
		            for (int cZ = chunkZ; cZ < maxZ; cZ++) {
		            	for (int cX = chunkX; cX < maxX; cX++) {
			            	Chunk chunk = tile.getChunkWriter(cX, cZ);
			            	
			                Rivermap rivers = null;
	                    	for (int dz = 0; dz < 16; dz++) {
	                    		for (int dx = 0; dx < 16; dx++) {
		                    		int worldX = chunk.getBlockX() + dx;
		                    		int worldZ = chunk.getBlockZ() + dz;
		                    		Cell cell = chunk.getCell(dx, dz);
		                    		
			                        this.heightmap.applyTerrain(cell, worldX, worldZ);
			                        rivers = Rivermap.get(cell, rivers, this.heightmap);
			                        this.heightmap.applyRivers(cell, worldX, worldZ, rivers);
			                        this.heightmap.applyClimate(cell, worldX, worldZ, true);
			                    }
			                }
			            }
			        }
				}, ThreadPools.WORLD_GEN);
	        }
	    }
		return CompletableFuture.allOf(futures).thenApply((v) -> {
			// 1. Lấy thiết lập Sharpness từ TerrainSettings (giả sử bạn đã lưu nó trong Heightmap)
			Sharpness currentSharpness = this.heightmap.getSettings().terrain().sharpness;

			// 2. Chạy bộ lọc Sharpness trước để định hình độ nhọn
			new SharpnessFilter(currentSharpness).apply(tile, tile.getX(), tile.getZ(), 1);

			// 3. Sau đó mới chạy các Filter cũ (Erosion, Smoothing...)
			this.filters.apply(tile, true, applyOptionalFilters);
			return tile;
		});
	}
	
	public CompletableFuture<Tile> generateZoomed(float centerX, float centerZ, float zoom, boolean applyOptionalFilters) {
		Tile tile = this.makeTile(0, 0);
		CompletableFuture<?>[] futures = new CompletableFuture<?>[this.batchCount * this.batchCount];
        float translateX = centerX - this.tileSizeBlocks.size() * zoom / 2.0F;
        float translateZ = centerZ - this.tileSizeBlocks.size() * zoom / 2.0F;
		for (int batchZ = 0; batchZ < this.batchCount; batchZ++) {
			for (int batchX = 0; batchX < this.batchCount; batchX++) {
				int chunkX = batchX * this.batchSize;
				int chunkZ = batchZ * this.batchSize;
				futures[batchX * this.batchCount + batchZ] = CompletableFuture.runAsync(() -> {
			        int maxX = Math.min(this.tileSizeChunks.total(), chunkX + this.batchSize);
			        int maxZ = Math.min(this.tileSizeChunks.total(), chunkZ + this.batchSize);
			        for (int cZ = chunkZ; cZ < maxZ; cZ++) {
			            for (int cX = chunkX; cX < maxX; cX++) {
			            	Chunk chunk = tile.getChunkWriter(cX, cZ);
			            	
			                Rivermap rivers = null;
	                    	for (int dz = 0; dz < 16; dz++) {
	                    		for (int dx = 0; dx < 16; dx++) {
		                    		float worldX = (chunk.getBlockX() + dx) * zoom + translateX;
		                    		float worldZ = (chunk.getBlockZ() + dz) * zoom + translateZ;
		                    		Cell cell = chunk.getCell(dx, dz);
		                    		
			                        this.heightmap.applyTerrain(cell, worldX, worldZ);
			                        rivers = Rivermap.get(cell, rivers, this.heightmap);
			                        this.heightmap.applyRivers(cell, worldX, worldZ, rivers);
			                        this.heightmap.applyClimate(cell, worldX, worldZ, true);
			                    }
			                }
			            }
			        }
				}, ThreadPools.WORLD_GEN);
	        }
	    }
		return CompletableFuture.allOf(futures).thenApply((v) -> {
			this.filters.apply(tile, applyOptionalFilters);
			return tile;
		});
	}
    
	private Tile makeTile(int x, int z) {
		return new Tile(x, z, this.tileChunks, this.tileBorder, this.tileSizeBlocks, this.tileSizeChunks, this.cellPool.get(this.tileSizeBlocks.arraySize()), this.chunkPool.get(this.tileSizeChunks.arraySize()));
	}
	
    private static int getBatchSize(int batchCount, Size chunkSize) {
        int batchSize = chunkSize.total() / batchCount;
        if (batchSize * batchCount < chunkSize.total()) {
            ++batchSize;
        }
        return batchSize;
    }
}
