package net.prj3.world.worldgen.densityfunction.tile;

import java.util.Arrays;
import java.lang.foreign.MemorySegment;

import net.prj3.concurrent.Resource;
import net.prj3.concurrent.cache.SafeCloseable;
import net.prj3.world.worldgen.cell.Cell;
import net.prj3.world.worldgen.cell.CellLookup;
import net.prj3.world.worldgen.densityfunction.tile.filter.Filterable;

public class Tile implements SafeCloseable, Filterable, CellLookup {
	private int x;
    private int z;
	private int chunkX;
    private int chunkZ;
	private int size;
	private int border;
	private Size blockSize;
	private Size chunkSize;

    private final Resource<MemorySegment> cacheResource;
    private final Resource<Chunk[]> chunkResource;

    private final MemorySegment cacheSegment;
    private final Chunk[] chunks;

    public Tile(int x, int z, int size, int border, Size blockSize, Size chunkSize, Resource<MemorySegment> cacheResource, Resource<Chunk[]> chunkResource) {
        this.x = x;
        this.z = z;
        this.chunkX = x << size;
        this.chunkZ = z << size;
        this.size = size;
        this.border = border;
        this.blockSize = blockSize;
        this.chunkSize = chunkSize;
        this.cacheResource = cacheResource;
        this.chunkResource = chunkResource;
        this.cacheSegment = cacheResource.get();
        this.chunks = chunkResource.get();
    }
	
	public int getX() {
		return this.x;
	}
	
	public int getZ() {
		return this.z;
	}
	
	@Override
	public Cell lookup(int blockX, int blockZ) {
		int border = this.blockSize.border();
        int relBlockX = border + this.blockSize.mask(blockX);
        int relBlockZ = border + this.blockSize.mask(blockZ);
        int index = this.blockSize.indexOf(relBlockX, relBlockZ);
        return this.cache[index];
	}
	
	public Chunk getChunkWriter(int chunkX, int chunkZ) {
        int index = this.chunkSize.indexOf(chunkX, chunkZ);
        return this.computeChunk(index, chunkX, chunkZ);
	}
	
	public Chunk getChunkReader(int chunkX, int chunkZ) {
        int relChunkX = this.chunkSize.border() + this.chunkSize.mask(chunkX);
        int relChunkZ = this.chunkSize.border() + this.chunkSize.mask(chunkZ);
        int index = this.chunkSize.indexOf(relChunkX, relChunkZ);
        return this.computeChunk(index, chunkX, chunkZ);
	}
	
    public void iterate(Cell.Visitor visitor) {
        for (int dz = 0; dz < this.blockSize.size(); ++dz) {
            int z = this.blockSize.border() + dz;
            for (int dx = 0; dx < this.blockSize.size(); ++dx) {
                int x = this.blockSize.border() + dx;
                int index = this.blockSize.indexOf(x, z);
                Cell cell = this.cache[index];
                visitor.visit(cell, dx, dz);
            }
        }
    }

	@Override
	public int getBlockX() {
		return Size.chunkToBlock(this.chunkX);
	}

	@Override
	public int getBlockZ() {
		return Size.chunkToBlock(this.chunkZ);
	}

	@Override
	public Size getBlockSize() {
		return this.blockSize;
	}

	public Size getChunksSize() {
		return this.chunkSize;
	}

	@Override
	public long getBacking() {
		return this.cacheSegment;
	}

    @Override
    public long getAddress() {
        return this.cacheSegment.address();
    }

    @Override
    public Cell lookup(int blockX, int blockZ) {
        int borderVal = this.blockSize.border();
        int relBlockX = borderVal + this.blockSize.mask(blockX);
        int relBlockZ = borderVal + this.blockSize.mask(blockZ);
        int idx = this.blockSize.indexOf(relBlockX, relBlockZ);
        return new Cell(getCellSegment(idx));
    }

    @Override
    public MemorySegment setCellSegment(int index) {
        long offset = index *  Cell.LAYOUT.byteSize();
        return this.cacheSegment.asSlice(offset, Cell.LAYOUT.byteSize());
    }

    @Override
    public Cell getCellRaw(int x, int z) {
        int index = this.blockSize.indexOf(x, z);
        if (index < 0 || index >= this.blockSize.arraySize()) {
            return Cell.empty();
        }
        return new Cell(getCellSegment(index));
    }

	@Override
	public void close() {
        this.cacheSegment.fill((byte) 0);
        Arrays.fill(this.chunks, null);
		this.cacheResource.close();
		this.chunkResource.close();
	}
	
    private Chunk computeChunk(int index, int chunkX, int chunkZ) {
        Chunk chunk = this.chunks[index];
        if (chunk == null) {
            chunk = new Chunk(chunkX, chunkZ);
            this.chunks[index] = chunk;
        }
        return chunk;
    }
	
	public class Chunk {
        private int chunkX;
        private int chunkZ;
        private int blockX;
        private int blockZ;
        private int regionBlockX;
        private int regionBlockZ;

        public class Chunk {
            private final int chunkX, chunkZ;
            private final int blockX, blockZ;
            private final int regionBlockX, regionBlockZ;

            public Chunk(int regionChunkX, int regionChunkZ) {
                this.regionBlockX = regionChunkX << 4;
                this.regionBlockZ = regionChunkZ << 4;
                this.chunkX = Tile.this.chunkX + regionChunkX - Tile.this.border;
                this.chunkZ = Tile.this.chunkZ + regionChunkZ - Tile.this.border;
                this.blockX = this.chunkX << 4;
                this.blockZ = this.chunkZ << 4;
            }

            public MemorySegment getCellSegment(int blockX, int blockZ) {
                int relX = this.regionBlockX + (blockX & 0xF);
                int relZ = this.regionBlockZ + (blockZ & 0xF);
                int index = Tile.this.blockSize.indexOf(relX, relZ);
                return Tile.this.getCellSegment(index);
            }
        }
	}
}
