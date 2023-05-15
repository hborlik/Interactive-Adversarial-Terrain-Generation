#include <terrain/erosion_model2.h>

#include <cmath>
#include <algorithm>
#include <iostream>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

#include <resource/image.h>
#include <graphics/texture.h>
#include <terrain/terrain.h>
#include <util/box_utils.h>

namespace dirtbox::terrain {

struct Erosion2GPUUniforms {
    Erosion2GPUUniforms() : 
        water_sediment_capacity         {1.f},
        maximal_erosion_depth           {10.0f},
        step_time_constant              {0.05f},
        cell_size                       {30.0f},
        // soil_absorption                 {0.0f},
        // bedrock_erosion_base_value      {0.05f},
        rock_erosion_base_value         {0.05f},
        iterations                      {1000},
        rainfall                        {0.01f},
        terrain_elevation_scale         {100},
        virtual_pipe_area               {10},
        soil_suspension_rate            {0.5f},
        sediment_deposition_rate        {1},
        soil_softness_max               {0.1f},
        water_evaporation_rate          {0.015f},
        thermal_erosion_rate            {0.15f},
        talus_angle_tangent_coef        {0.8f},
        talus_angle_tangent_bias        {0.1f} {
        u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 5);
    }

    void submit() {
        bgfx::setUniform(u_params, params, 5);
    }

    void toParameterSet(util::ParameterCollection<float>& parameters) {
        parameters.addParameter("water_sediment_capacity", 0.1f, 3.f, water_sediment_capacity);
        parameters.addParameter("maximal_erosion_depth", 0, 40, maximal_erosion_depth);
        parameters.addParameter("iterations", 1, 1, iterations);
        parameters.addParameter("step_time_constant", 0.01f, 10.0f, step_time_constant);
        // rainfall in meters per year
        parameters.addParameter("rainfall", 0.0001f, 0.5f, rainfall);
        parameters.addParameter("cell_size", 0.5f, 150.0f, cell_size);

        // soil absorbtion coef
        // parameters.addParameter("soil_absorption", 0.0f, 1.0f, soil_absorption);

        // layer parameters
        parameters.addParameter("rock_erosion_base_value", 0.0001f, 0.1f, rock_erosion_base_value);
        // parameters.addParameter("bedrock_erosion_base_value", 0.0001f, 0.1f, bedrock_erosion_base_value);

        parameters.addParameter("terrain_elevation_scale", 1, 500, terrain_elevation_scale);
        parameters.addParameter("virtual_pipe_area", 0.1, 60, virtual_pipe_area);
        parameters.addParameter("soil_suspension_rate", 0.1f, 2.f, soil_suspension_rate);
        parameters.addParameter("sediment_deposition_rate", 0.1f, 3.f, sediment_deposition_rate);
        parameters.addParameter("soil_softness_max", 0.1, 1, soil_softness_max);
        parameters.addParameter("water_evaporation_rate", 0, 0.05f, water_evaporation_rate);
        parameters.addParameter("thermal_erosion_rate", 0, 3, thermal_erosion_rate);
        parameters.addParameter("talus_angle_tangent_coef", 0, 1, talus_angle_tangent_coef);
        parameters.addParameter("talus_angle_tangent_bias", 0, 1, talus_angle_tangent_bias);
    }

    void fromParameterSet(const util::ParameterCollection<float>& p) {
        water_sediment_capacity     = p.getParam("water_sediment_capacity");
        maximal_erosion_depth       = p.getParam("maximal_erosion_depth");
        iterations                  = p.getParam("iterations");
        step_time_constant          = p.getParam("step_time_constant");
        rainfall                    = p.getParam("rainfall");
        cell_size                   = p.getParam("cell_size");
        // soil_absorption             = p.getParam("soil_absorption");
        rock_erosion_base_value     = p.getParam("rock_erosion_base_value");
        // bedrock_erosion_base_value  = p.getParam("bedrock_erosion_base_value");
        terrain_elevation_scale     = p.getParam("terrain_elevation_scale");
        virtual_pipe_area           = p.getParam("virtual_pipe_area");
        soil_suspension_rate        = p.getParam("soil_suspension_rate");
        sediment_deposition_rate    = p.getParam("sediment_deposition_rate");
        soil_softness_max           = p.getParam("soil_softness_max");
        water_evaporation_rate      = p.getParam("water_evaporation_rate");
        thermal_erosion_rate        = p.getParam("thermal_erosion_rate");
        talus_angle_tangent_coef    = p.getParam("talus_angle_tangent_coef");
        talus_angle_tangent_bias    = p.getParam("talus_angle_tangent_bias");
    }

    bgfx::UniformHandle u_params;

    union
    {
        struct
        {
            float water_sediment_capacity;
            float maximal_erosion_depth;
            float step_time_constant;
            float cell_size;

            float unused;
            float bedrock_erosion_base_value;
            float rock_erosion_base_value;
            float iterations;

            float rainfall;
            float terrain_elevation_scale;
            float virtual_pipe_area;
            float soil_suspension_rate;

            float sediment_deposition_rate;
            float soil_softness_max;
            float water_evaporation_rate;
            float thermal_erosion_rate;

            float talus_angle_tangent_coef;
            float talus_angle_tangent_bias;
        };

        float params[20];
    };
};

class Erosion2GPUImpl {
public:
    bool A_B = true;

    bgfx::ProgramHandle erosion_program;

    bgfx::TextureHandle elevation_data_a    {bgfx::kInvalidHandle};
    bgfx::TextureHandle outflows_data_a     {bgfx::kInvalidHandle};
    bgfx::TextureHandle velocity_data_a     {bgfx::kInvalidHandle};

    bgfx::TextureHandle elevation_data_b    {bgfx::kInvalidHandle};
    bgfx::TextureHandle outflows_data_b     {bgfx::kInvalidHandle};
    bgfx::TextureHandle velocity_data_b     {bgfx::kInvalidHandle};

    bgfx::TextureHandle soil_flows_1        {bgfx::kInvalidHandle};
    bgfx::TextureHandle soil_flows_2        {bgfx::kInvalidHandle};

    Erosion2GPUUniforms uniforms;

    int w, h;

    ~Erosion2GPUImpl() {
        if (bgfx::isValid(elevation_data_a))
            bgfx::destroy(elevation_data_a);
        if (bgfx::isValid(outflows_data_a))
            bgfx::destroy(outflows_data_a);
        if (bgfx::isValid(velocity_data_a))
            bgfx::destroy(velocity_data_a);

        if (bgfx::isValid(elevation_data_b))
            bgfx::destroy(elevation_data_b);
        if (bgfx::isValid(outflows_data_b))
            bgfx::destroy(outflows_data_b);
        if (bgfx::isValid(velocity_data_b))
            bgfx::destroy(velocity_data_b);

        if (bgfx::isValid(soil_flows_1))
            bgfx::destroy(soil_flows_1);
        if (bgfx::isValid(soil_flows_2))
            bgfx::destroy(soil_flows_2);
    }

    bgfx::TextureHandle getOutputElevationData() {
        return A_B ? elevation_data_b : elevation_data_a;
    }

    void loadPrograms() {
        erosion_program = bgfx::createProgram(loadShader("cs_model2_erosion"), true);
    }

    void loadTextures() {
        if (bgfx::isValid(elevation_data_a)) {
            bgfx::destroy(elevation_data_a);
        }
        elevation_data_a = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(outflows_data_a)) {
            bgfx::destroy(outflows_data_a);
        }
        outflows_data_a = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(velocity_data_a)) {
            bgfx::destroy(velocity_data_a);
        }
        velocity_data_a = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(elevation_data_b)) {
            bgfx::destroy(elevation_data_b);
        }
        elevation_data_b = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(outflows_data_b)) {
            bgfx::destroy(outflows_data_b);
        }
        outflows_data_b = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(velocity_data_b)) {
            bgfx::destroy(velocity_data_b);
        }
        velocity_data_b = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(soil_flows_1)) {
            bgfx::destroy(soil_flows_1);
        }
        soil_flows_1 = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);

        if (bgfx::isValid(soil_flows_2)) {
            bgfx::destroy(soil_flows_2);
        }
        soil_flows_2 = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);
    }

    void init(const graphics::Texture& terr) {
        w = terr.getWidth();
        h = terr.getHeight();
        loadTextures();
        bgfx::blit(3, elevation_data_a, 0, 0, terr.getHandle());
    }

    bool copyTerrainTo(const graphics::Texture& terr) {
        bgfx::blit(3, terr.getHandle(), 0, 0, getOutputElevationData());
    }

    void submit() {
        if (A_B) {
            bgfx::setImage(0, elevation_data_a, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(3, elevation_data_b, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);
            
            bgfx::setImage(1, outflows_data_a, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(4, outflows_data_b, 0,        bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);

            bgfx::setImage(2, velocity_data_a, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(5, velocity_data_b, 0,        bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);
        } else {
            bgfx::setImage(0, elevation_data_b, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(3, elevation_data_a, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);
            
            bgfx::setImage(1, outflows_data_b, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(4, outflows_data_a, 0,        bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);

            bgfx::setImage(2, velocity_data_b, 0,        bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(5, velocity_data_a, 0,        bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);
        }

        bgfx::setImage(6, soil_flows_1, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);
        bgfx::setImage(7, soil_flows_2, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);

        uniforms.submit();
        bgfx::dispatch(3, erosion_program, w, h);

        A_B = !A_B;
    }
};

Erosion2SimulationGPU::Erosion2SimulationGPU(std::shared_ptr<Terrain> target)
    : Erosion{"Erosion2SimulationGPU", std::move(target)}, m_gpu{std::make_unique<Erosion2GPUImpl>()}
{
    m_gpu->uniforms.toParameterSet(parameters);

    m_gpu->loadPrograms();
    
}

void Erosion2SimulationGPU::startErosionTask() {
    if (!m_isRunning) {
        m_gpu->uniforms.fromParameterSet(parameters);
        m_gpu->init(target->getTerrainTexture());
        m_itercounter = 0;
        m_gpu->A_B = true;
        m_isRunning = true;
    }
}

void Erosion2SimulationGPU::stopErosionTask() {
    m_isRunning = false;
}

float Erosion2SimulationGPU::getProgress() const {
    return m_itercounter / (m_gpu->uniforms.iterations);
}

bool Erosion2SimulationGPU::isRunning() const {
    return m_isRunning;
}

void Erosion2SimulationGPU::update() {
    if (m_isRunning) {
        if (m_itercounter > m_gpu->uniforms.iterations) {
            m_isRunning = false;
        } else {
            run_erosion();
        }
    }
}

bgfx::TextureHandle Erosion2SimulationGPU::getDisplayHeightmap() {
    return m_gpu->A_B ? m_gpu->elevation_data_a : m_gpu->elevation_data_b;
}

void Erosion2SimulationGPU::run_erosion() {
    if (m_isRunning) {
        //bgfx::touch(3);
        
        m_gpu->submit();
        m_gpu->copyTerrainTo(target->getTerrainTexture());
        m_itercounter++;
    }
}

}