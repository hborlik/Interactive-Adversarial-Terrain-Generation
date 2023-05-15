/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-11
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_ETES_EROSION_H
#define DIRTBOX_ETES_EROSION_H

#include <future>
#include <atomic>
#include <cmath>

#include <terrain/erosion.h>
#include <util/task.h>

namespace dirtbox::terrain {

struct ETESCellData {
    float R;    // rock
    float C;    // sand
    float H;    // humus
    float Sz;   // bedrock elevation, initialized by ModelSubstate

    // trees count, age, height
    //float Tc, Ta, Th;

    // shrubs count, age, height
    //float Sc, Sa, Sh;

    // grass density
    //float Gd; // [0, 1]

    // moisture content
    float M;
    float pad[3];
    
    // average daily sunlight exposure
    //float I;

    // dead vegitation, and total vegitation density
    //float D, V;

    bool isNanOrInf() const {
        //float v = R+C+H+Sz+Tc+Ta+Th+Sc+Sa+Sh+Gd+M+I+D+V;
        float v = R+C+H+Sz+M;
        return std::isnan(v) && std::isinf(v);
    }

    bool isValid() const {
        return 
            R >= 0.0f &&
            C >= 0.0f &&
            H >= 0.0f &&
            Sz >= 0.0f &&
            // Tc >= 0.0f &&
            // Ta >= 0.0f &&
            // Th >= 0.0f &&
            // Sc >= 0.0f &&
            // Sa >= 0.0f &&
            // Sh >= 0.0f &&
            // Gd >= 0.0f &&
            M >= 0.0f;// &&
            // I >= 0.0f &&
            // D >= 0.0f &&
            // V >= 0.0f;
    }

    void clamp() {
        R  = std::max(R , 0.f);
        C  = std::max(C , 0.f);
        H  = std::max(H , 0.f);
        Sz = std::max(Sz, 0.f);
        // Tc = std::max(Tc, 0.f);
        // Ta = std::max(Ta, 0.f);
        // Th = std::max(Th, 0.f);
        // Sc = std::max(Sc, 0.f);
        // Sa = std::max(Sa, 0.f);
        // Sh = std::max(Sh, 0.f);
        // Gd = std::max(Gd, 0.f);
        M  = std::max(M , 0.f);
        // I  = std::max(I , 0.f);
        // D  = std::max(D , 0.f);
        // V  = std::max(V , 0.f);
    }
};

struct ETESModelParameters {
    int   iterations                  = 5;
    float time_step_years             = 1.0f;
    float rainfall                    = 10.0f;
    float cell_size                   = 30.0f;
    float water_sediment_capacity_p   = 0.05f;
    float soil_absorption             = 0.2f;
    float slope_threshold_sediment_lift= 0.1f;
    float runoff_time_step_hours      = 1.0f;

    float humus_water_capacity_p      = 0.8f;
    float sand_water_capacity_p       = 0.3f;
    float rock_water_capacity_p       = 0.05f;
    float rock_erosion_base_value     = 0.0005f;
    float bedrock_water_capacity_p    = 0.01f;
    float bedrock_erosion_base_value  = 0.0005f;



    void toParameterSet(util::ParameterCollection<float>& parameters) {
        parameters.addParameter("iterations", 1, 1, iterations);
        parameters.addParameter("time_step_years", 0.05f, 10.0f, time_step_years);
        // rainfall in meters per year
        parameters.addParameter("rainfall", 0.0f, 15.0f, rainfall);
        parameters.addParameter("cell_size", 0.5f, 150.0f, cell_size);

        // percentage of water volume that can be sediment
        parameters.addParameter("water_sediment_capacity_p", 0.0f, 1.0f, water_sediment_capacity_p);
        // soil absorbtion coef
        parameters.addParameter("soil_absorption", 0.0f, 1.0f, soil_absorption);
        // threshold for sediment lift on a slope (grade) 
        parameters.addParameter("slope_threshold_sediment_lift", 0.01f, 1.0f, slope_threshold_sediment_lift);

        parameters.addParameter("runoff_time_step_hours", 0.01f, 100.0f, runoff_time_step_hours);

        // layer parameters
        parameters.addParameter("humus_water_capacity_p", 0.01f, 1.0f, humus_water_capacity_p);
        parameters.addParameter("sand_water_capacity_p", 0.01f, 1.0f, sand_water_capacity_p);
        parameters.addParameter("rock_water_capacity_p", 0.01f, 1.0f, rock_water_capacity_p);
        parameters.addParameter("rock_erosion_base_value", 0.0001f, 0.1f, rock_erosion_base_value);
        parameters.addParameter("bedrock_water_capacity_p", 0.01f, 1.0f, bedrock_water_capacity_p);
        parameters.addParameter("bedrock_erosion_base_value", 0.0001f, 0.1f, bedrock_erosion_base_value);
    }

    void fromParameterSet(const util::ParameterCollection<float>& p) {
        iterations                  = p.getParam("iterations");
        time_step_years             = p.getParam("time_step_years");
        rainfall                    = p.getParam("rainfall");
        cell_size                   = p.getParam("cell_size");
        water_sediment_capacity_p   = p.getParam("water_sediment_capacity_p");
        soil_absorption             = p.getParam("soil_absorption");
        humus_water_capacity_p      = p.getParam("humus_water_capacity_p");
        sand_water_capacity_p       = p.getParam("sand_water_capacity_p");
        rock_water_capacity_p       = p.getParam("rock_water_capacity_p");
        rock_erosion_base_value     = p.getParam("rock_erosion_base_value");
        bedrock_water_capacity_p    = p.getParam("bedrock_water_capacity_p");
        bedrock_erosion_base_value  = p.getParam("bedrock_erosion_base_value");
        slope_threshold_sediment_lift=p.getParam("slope_threshold_sediment_lift");
        runoff_time_step_hours      = p.getParam("runoff_time_step_hours");
    }

    float getCellElevation(const ETESCellData& cell) const {return cell.Sz + cell.R + cell.C + cell.H;}

    float getCellGranularDepth(const ETESCellData& cell) const {return cell.R + cell.C + cell.H;}
    /**
     * @brief Get the Cell Vegitation Density. Kg / dA
     * 
     * @param  cell
     * @return float 
     */
    float getCellVegitationDensity(const ETESCellData& cell) const {
        // TODO consider other vegitation types (tree and shrubs)
        //return cell.Gd;
        return 0;
    }
};

class EcosystemTerrainErosionSimulation : public Erosion {
public:
    EcosystemTerrainErosionSimulation();

    void startErosionTask() override;
    void stopErosionTask() override;

    float getProgress() const override;
    bool isRunning() const override;
    void update() override;

private:
    void run_erosion(std::atomic_uint32_t&, std::future<void>) const;

    struct ErosionTaskData;
    std::shared_ptr<ErosionTaskData> task_data;
    util::AsyncProgressTask erosion_task;
};

class EcosystemTerrainErosionSimulationGPU : public Erosion {
public:
    EcosystemTerrainErosionSimulationGPU(std::shared_ptr<Terrain> target);

    void init();

    void startErosionTask() override;
    void stopErosionTask() override;

    float getProgress() const override;
    bool isRunning() const override;
    void update() override;

    void read();

private:
    void run_erosion();

    struct ErosionTaskData;
    ETESModelParameters params;
    bool m_isRunning = false;
    uint32_t m_itercounter = 0;
    std::shared_ptr<class ETESGPUImpl> m_etesgpu;
};

}

#endif // DIRTBOX_ETES_EROSION_H