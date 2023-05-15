// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////// EcosystemTerrainErosionSimulation ///////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// /*
//     based on https://dl.acm.org/doi/pdf/10.1145/3072959.3073667
// */

// #include <terrain/etes_erosion.h>

// #include <cmath>
// #include <algorithm>
// #include <assert.h>

// #include <util/vec.h>
// #include <terrain/erosion_util.h>

// using namespace std;
// using namespace util;

// namespace dirtbox::terrain {

// // erosion task types
// using ETESGrid  = ModelSubstate<ETESCellData>;
// using ETESN     = ETESGrid::Neighborhood;

// struct EcosystemTerrainErosionSimulation::ErosionTaskData {
//     ErosionTaskData(int W, int H, float elevationMax) : 
//         cellData{W, H, elevationMax} {}
//     ModelSubstate<ETESCellData> cellData;
// };


// EcosystemTerrainErosionSimulation::EcosystemTerrainErosionSimulation() 
//     : erosion_task{AsyncProgressTask::TaskFnType::create<EcosystemTerrainErosionSimulation, &EcosystemTerrainErosionSimulation::run_erosion>(this)} {
//     ETESModelParameters _m;
//     _m.toParameterSet(parameters);
// }


// class RunoffEvent {
// public:
//     ETESModelParameters& params;
//     const float step_time_constant;

//     // water volume
//     float w;
//     float sediment_humus;
//     float sediment_sand;
//     float sediment_rock;

//     void reset() {
//         w = 0;
//         sediment_humus = 0;
//         sediment_sand = 0;
//         sediment_rock = 0;
//     }

//     float sedimentCapacity() const {
//         return w * params.water_sediment_capacity_p;
//     }

//     float currentSediment() const {
//         return sediment_humus + sediment_sand + sediment_rock;
//     }

//     float sedimentSaturation() const {
//         return currentSediment() / sedimentCapacity();
//     }

//     float cellMoistureCapacity(const ETESCellData& cell) {
//         return params.humus_water_capacity_p * cell.H + 
//             params.sand_water_capacity_p * cell.C +
//             params.rock_water_capacity_p * cell.R + 
//             params.bedrock_water_capacity_p * cell.Sz;
//     }

//     float sedimentDepositionCalculation(float available_material, float slope, float veg_density) {
//         float deposit = step_time_constant * (1.f - slope);
//         deposit *= available_material;
//         deposit = min(deposit, available_material);
//         return deposit;
//     }

//     /**
//      * @brief 
//      * 
//      * @param available_material    maximum erodable sediment
//      * @param slope                 grade logistic value
//      * @param veg_density           vegitation density scalar
//      * @return float 
//      */
//     float sedimentLiftCalculation(float available_material, float slope, float veg_density) {
//         float lift = step_time_constant * slope;
//         lift *= available_material;
//         lift = min(lift, available_material);
//         return lift;
//     }

//     /**
//      * @brief solid material erosion
//      * 
//      * 
//      * @param erosion_base 
//      * @param available_material
//      * @param slope 
//      * @param w         
//      * @param sediment_saturation       [0, 1]
//      * @return float    meters of erosion 
//      */
//     float erosion(float erosion_base, float available_material, float slope, float w, float sediment_saturation, float veg_density, float granular_depth) {
//         float erosion = step_time_constant * erosion_base * w * 
//             (slope + 0.2f * logistic_between(sediment_saturation, 0.f, 1.f)) * 
//             (1.f - logistic_between(granular_depth, 0.1f, 2.f));
//         erosion = min(available_material, erosion);
//         return erosion;
//     }
    
//     /**
//      * @brief performs single step of the runoff event
//      * 
//      * @param p cell to run the event on
//      * @param N neighborhood of target cell
//      * @return int next cell direction
//      */
//     int stepRunoffEvent(ETESCellData& p, const ETESN& N) {
//         const float ctcd = sqrt(params.cell_size*params.cell_size);
//         float c_elevation = params.getCellElevation(p);
//         float slopes[8] = {};
//         // eliminate cells that are higher
//         for (uint32_t i = 0; i < N.adjacent.size(); ++i) {
//             const auto& cell = N.adjacent[i];
//             if (cell) {
//                 float n_z = params.getCellElevation(*cell);
//                 if (n_z < c_elevation)
//                     slopes[i] = slope(c_elevation, n_z, ctcd);
//             }
//         }
//         int r_dir = random_weighted_pick(std::vector(slopes, slopes + 8));
//         if (r_dir != -1) { // picking a new direction
//             float grade = abs(slopes[r_dir]);
//             float r_slope = logistic_between(grade, -4.f, 5.f, 10.0f);

//             // absorption
//             float absorb = step_time_constant * params.soil_absorption * w * (1.f - r_slope);
//             absorb = min(absorb, cellMoistureCapacity(p) - p.M);
//             w -= absorb;
//             p.M += absorb;

//             // erosion interactions
//             const float cell_vd = params.getCellVegitationDensity(p);
//             float sand;
//             float humus;
//             float rock;
//             if (grade > params.slope_threshold_sediment_lift) { // material transport
//                 // sediment lift
//                 sand = sedimentLiftCalculation(p.C, r_slope, cell_vd);
//                 humus = sedimentLiftCalculation(p.H, r_slope, cell_vd);
//                 rock = sedimentLiftCalculation(p.R, r_slope, cell_vd);

//             } else {
//                 // sediment deposition
//                 sand = -sedimentDepositionCalculation(sediment_sand, r_slope, cell_vd);
//                 humus = -sedimentDepositionCalculation(sediment_humus, r_slope, cell_vd);
//                 rock = -sedimentDepositionCalculation(sediment_rock, r_slope, cell_vd);
//             }

//             // erosion bedrock -> rock -> sand
//             float b_r = erosion(params.bedrock_erosion_base_value, p.Sz, r_slope, w, sedimentSaturation(), cell_vd, params.getCellGranularDepth(p));
//             float r_c = erosion(params.rock_erosion_base_value, p.R, r_slope, w, sedimentSaturation(), cell_vd, params.getCellGranularDepth(p));
//             p.Sz -= b_r;
//             p.R += b_r - r_c;
//             p.C += r_c;

//             const float capacity = sedimentCapacity();
//             float total = sand + humus + rock;
//             float current = currentSediment();
//             float remaining = capacity - current;
//             if (current > capacity) {
//                 sand = sediment_sand / current * remaining;
//                 humus = sediment_humus / current * remaining;
//                 rock = sediment_rock / current * remaining;
//             } else if (total > remaining) {
//                 sand = sand / (total + 1e-4f) * remaining;
//                 humus = humus / (total + 1e-4f) * remaining;
//                 rock = rock / (total + 1e-4f) * remaining;
//             }

//             sediment_sand += sand;
//             p.C -= sand;

//             sediment_humus += humus;
//             p.H -= humus;

//             sediment_rock += rock;
//             p.R -= rock;

//             assert(!p.isNanOrInf());
//         }
//         return r_dir;
//     }

//     void doRunoffEvent(ModelSubstate<ETESCellData>& input_grid, vec2i point, float w) {
//         ETESCellData* p = input_grid.safeGet(point);
//         reset();
//         this->w = w;
//         if (p) {
//             for (int i = 0; i < 25; ++i) {
//                 int dir = stepRunoffEvent(*p, input_grid.neighborhoodOf(point.x(), point.y()));
//                 p->clamp();
//                 if (dir != -1 && w > 0.0f) {
//                     point += ModelSubstate<ETESCellData>::Neighborhood::directions[dir];
//                     p = input_grid.safeGet(point);
//                     if (!p) break;
//                 } else {
//                     // deposit all remaining
//                     p->C += sediment_sand;
//                     p->H += sediment_humus;
//                     p->R += sediment_rock;
//                     break;
//                 }
//             }
//         }
//     }
// };

// struct LandslideEvent {

// };



// void EcosystemTerrainErosionSimulation::startErosionTask(std::shared_ptr<graphics::Texture> terr) {
//     if (erosion_task.isDone()) {
//         task_data = std::make_unique<ErosionTaskData>(terr->getWidth(), terr->getHeight(), 500.0f);
//         task_data->cellData.copyElevation(terr);

//         erosion_task.start();
//     }
// }

// float EcosystemTerrainErosionSimulation::getProgress() const {
//     return erosion_task.getProgress();
// }

// bool EcosystemTerrainErosionSimulation::isRunning() const {
//     return !erosion_task.isDone();
// }

// bool EcosystemTerrainErosionSimulation::getElevationData(std::shared_ptr<graphics::Texture> terr) {
//     if (erosion_task.join()) {
//         task_data->cellData.copyElevationTo(terr);
//         return true;
//     }
//     return false;
// }

// void EcosystemTerrainErosionSimulation::update() {
    
// }

// // on separate thread
// void EcosystemTerrainErosionSimulation::run_erosion(std::atomic_uint32_t& progress, std::future<void> kill_me) const {
//     auto& mdata = task_data->cellData;

//     //ModelSubstate<ETESCellData> mdata2{mdata};

//     const int iterations = (int)parameters.getParam("iterations");
//     ETESModelParameters params{};
//     params.fromParameterSet(parameters);
//     RunoffEvent runoff{params, 1.0f};

//     bool A_B = true;
//     for (int i = 0; i < iterations; ++i) {
//         // simulation step
//         for(int y = 0; y < mdata.height; ++y) {
//             for(int x = 0; x < mdata.width; ++x) {
//                 int xr = (float)random() / RAND_MAX * mdata.width;
//                 int yr = (float)random() / RAND_MAX * mdata.height;
//                 runoff.doRunoffEvent(mdata, vec2i{xr, yr}, params.rainfall);
//             }
//             progress.store((float)(i * mdata.height + y) / (iterations * mdata.width) * std::numeric_limits<uint32_t>::max());
//         }
//         // A_B = !A_B;
//     }

//     // update elevations with granular info 
//     for(int y = 0; y < mdata.height; ++y)
//         for(int x = 0; x < mdata.width; ++x) {
//             mdata.at(x, y).Sz += params.getCellGranularDepth(mdata.at(x, y));
//         }
// }

// } // namespace dirtbox::terrain