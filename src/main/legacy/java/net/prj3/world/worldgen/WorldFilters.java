package net.prj3.world.worldgen;

import java.util.function.IntFunction;
import net.prj3.world.worldgen.densityfunction.tile.Modifier;
import net.prj3.data.worldgen.preset.settings.FilterSettings;
import net.prj3.world.worldgen.densityfunction.tile.Tile;
import net.prj3.world.worldgen.densityfunction.tile.filter.*;

public class WorldFilters {
    private Smoothing smoothing;
    private Steepness steepness;
    private BeachDetect beach;
    private NoiseCorrection corrections;
    private FilterSettings settings;
    
    // 1. Thêm field cho CoastalErosion
    private final WorldErosion<CoastalErosion> coastalErosion;
    
    private final WorldErosion<ForceErosion> forceErosion;
    private final WorldErosion<AeroErosion> aeroErosion;
    private final WorldErosion<GlacialErosionFull> glacialErosion;
    private final WorldErosion<Erosion> hydraulicErosion;
    private final WorldErosion<ThermalErosion> thermalErosion;
    private final WorldErosion<LandSlide> landSlide;
    private final WorldErosion<AdvancedSoilFluction> soilFluction;
    private final WorldErosion<AdvancedSubsurfaceFlow> advancedSubsurfaceFlow;
    private final WorldErosion<PhysicalSnowAvalanche> snowAvalanche;
    private final WorldErosion<Attrition> attrition;

    private final int erosionIterations;
    private final int smoothingIterations;
    private final int glacialIterations;
    private final int soilIterations;
    private final int snowAvalancheIterations;
    
    public WorldFilters(GeneratorContext context) {
        this.settings = context.preset.filters();

        this.beach = BeachDetect.make(context);
        this.smoothing = Smoothing.make(context.preset.filters().smoothing, context.levels);
        this.steepness = Steepness.make(1, 10.0F, context.levels);
        this.corrections = new NoiseCorrection(context.levels);
        
        // 2. Khởi tạo CoastalErosion bằng cách lấy Factory từ context
        // Đảm bảo trg GeneratorContext đã có: public final IntFunction<CoastalErosion> coastalErosionFactory;
        this.coastalErosion = new WorldErosion<>(
            context.coastalErosionFactory, 
            (c, size) -> c.getSize() == size
        );

        // Khởi tạo các bộ lọc khác (giữ nguyên logic fetch từ context của bạn)
        this.forceErosion = new WorldErosion<>(context.forceErosionFactory, (f, size) -> f.getSize() == size);
        this.aeroErosion = new WorldErosion<>(context.aeroErosionFactory, (a, size) -> a.getSize() == size);
        this.glacialErosion = new WorldErosion<>(context.glacialErosionFactory, (g, size) -> g.getSize() == size);
        this.hydraulicErosion = new WorldErosion<>(context.hydraulicErosionFactory, (e, size) -> e.getSize() == size);
        this.thermalErosion = new WorldErosion<>(context.thermalErosionFactory, (t, size) -> t.getSize() == size);
        this.landSlide = new WorldErosion<>(context.landSlideFactory, (l, size) -> l.getSize() == size);
        this.soilFluction = new WorldErosion<>(context.soilFluctionFactory, (s, size) -> s.getSize() == size);
        this.snowAvalanche = new WorldErosion<>(context.physicalSnowAvalancheFactory, (s, size) -> s.getSize() == size);
        this.advancedSubsurfaceFlow = new WorldErosion<>(context.advancedSubsurfaceFlowFactory, (s, size) -> s.getSize() == size);
        this.attrition = new WorldErosion<>(context.attrition, (l, size) -> l.getSize() == size);
        this.soilIterations = settings.soilFluction.iterations;
        this.glacialIterations = settings.glacial.iterations;
        this.erosionIterations = context.preset.filters().erosion.dropletsPerChunk;
        this.smoothingIterations = context.preset.filters().smoothing.iterations;
    }
    
    public FilterSettings getSettings() {
        return this.settings;
    }
    
    public void apply(Tile tile, boolean optionalFilters) {
        int regionX = tile.getX();
        int regionZ = tile.getZ();
        
        if (optionalFilters) {
            this.applyOptionalFilters(tile, regionX, regionZ);
        }
        this.applyRequiredFilters(tile, regionX, regionZ);

        if(optionalFilters) {
            this.applyCorrections(tile, regionX, regionZ);
        }
    }
    
    private void applyRequiredFilters(Filterable map, int seedX, int seedZ) {
        this.steepness.apply(map, seedX, seedZ, 1);
        this.beach.apply(map, seedX, seedZ, 1);
    }
    
    private void applyOptionalFilters(Filterable map, int seedX, int seedZ) {
        int size = map.getBlockSize().total();

        // Bước 1: Nội lực
        this.forceErosion.get(size).apply(map, seedX, seedZ, 1);
        
        // 3. Chèn CoastalErosion vào ngay sau ForceErosion
        this.coastalErosion.get(size).apply(map, seedX, seedZ, 2);
        
        // Bước 2: Khí quyển
        this.aeroErosion.get(size).apply(map, seedX, seedZ, 4);
        
        // Bước 3: Băng hà
        this.glacialErosion.get(size).apply(map, seedX, seedZ, this.glacialIterations);

        // Bước 4: Thủy văn (Sửa lại tên gọi cho khớp với biến hydraulicErosion)
        this.hydraulicErosion.get(size).apply(map, seedX, seedZ, this.erosionIterations);

        // Bước 5: Trọng lực & Sạt lở
        this.thermalErosion.get(size).apply(map, seedX, seedZ, 2);
        this.landSlide.get(size).apply(map, seedX, seedZ, 1);
        this.attrition.get(size).apply(map, seedX, seedZ, this.AttritionIterations)
        this.soilFluction.get(size).apply(map, seedX, seedZ, 4);
        this.snowAvalanche.get(size).apply(map, seedX, seedZ, this.snowAvalancheIterations);
        
        // Bước 6: Làm mượt
        this.smoothing.apply(map, seedX, seedZ, this.smoothingIterations);
    }
    
    public void applyCorrections(Filterable map, int seedX, int seedZ) {
        this.corrections.apply(map, seedX, seedZ, 1);
    }
}