#include <terrain/etes_erosion.h>

#include <cmath>
#include <algorithm>
#include <iostream>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

#include <resource/image.h>
#include <graphics/texture.h>
#include <util/box_utils.h>

using namespace util;

namespace dirtbox::terrain {

struct ETESGPUUniforms {
    ETESGPUUniforms() {
        u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 4);
        u_rand_offset = bgfx::createUniform("u_rand_offset", bgfx::UniformType::Vec4, 0);
    }

    void submit() {
        bgfx::setUniform(u_params, params, 4);
        bgfx::setUniform(u_rand_offset, rand_offset, 1);
    }

    void applyParameter(const ETESModelParameters& p) {
        water_sediment_capacity_p       = p.water_sediment_capacity_p;
        humus_water_capacity_p          = p.humus_water_capacity_p;
        sand_water_capacity_p           = p.sand_water_capacity_p;
        rock_water_capacity_p           = p.rock_water_capacity_p;
        bedrock_water_capacity_p        = p.bedrock_water_capacity_p;
        step_time_constant              = p.time_step_years;
        cell_area                       = std::sqrt(p.cell_size * p.cell_size);
        soil_absorption                 = p.soil_absorption;
        slope_threshold_sediment_lift   = p.slope_threshold_sediment_lift;
        bedrock_erosion_base_value      = p.bedrock_erosion_base_value;
        rock_erosion_base_value         = p.rock_erosion_base_value;
        iterations                      = p.iterations;
        rainfall                        = p.rainfall;
    }

    bgfx::UniformHandle u_params;
    bgfx::UniformHandle u_rand_offset;

    union {
        float rand_offset[4];
    };

    union
    {
        struct
        {
            float water_sediment_capacity_p;
            float humus_water_capacity_p;
            float sand_water_capacity_p;
            float rock_water_capacity_p;
            float bedrock_water_capacity_p;
            float step_time_constant;
            float cell_area;            // sqrt(cell_size^2)
            float soil_absorption;
            float slope_threshold_sediment_lift;
            float bedrock_erosion_base_value;
            float rock_erosion_base_value;
            float iterations;
            float rainfall;
        };

        float params[14];
    };
};

class ETESGPUImpl {
public:
    // bgfx::Encoder* m_encoder;
    bool A_B = true;

    bgfx::ProgramHandle etes_erosion_program;
    bgfx::ProgramHandle etes_erosion_init;

    bgfx::VertexLayout computeVertexLayoutEvent;

    bgfx::DynamicVertexBufferHandle computeEventBufferA {bgfx::kInvalidHandle};
    bgfx::DynamicVertexBufferHandle computeEventBufferB {bgfx::kInvalidHandle};

    bgfx::TextureHandle elevation_data  {bgfx::kInvalidHandle};
    bgfx::TextureHandle ground_data     {bgfx::kInvalidHandle};
    bgfx::TextureHandle cell_data_a     {bgfx::kInvalidHandle};
    bgfx::TextureHandle cell_data_b     {bgfx::kInvalidHandle};
    bgfx::TextureHandle rand_data       {bgfx::kInvalidHandle};
    bgfx::TextureHandle computeListBufferA  {bgfx::kInvalidHandle};
    bgfx::TextureHandle computeListBufferB  {bgfx::kInvalidHandle};

    ETESGPUUniforms uniforms;

    resource::ImageDataRef data;
    int w, h;

    ETESGPUImpl() {
        computeVertexLayoutEvent.begin()
            .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float)
            .end();
    }

    ~ETESGPUImpl() {
        bgfx::destroy(etes_erosion_program);
        bgfx::destroy(elevation_data);
        bgfx::destroy(ground_data);
        bgfx::destroy(cell_data_a);
        bgfx::destroy(cell_data_b);
        bgfx::destroy(rand_data);
        bgfx::destroy(computeEventBufferA);
        bgfx::destroy(computeEventBufferB);
        bgfx::destroy(computeListBufferA);
        bgfx::destroy(computeListBufferB);
    }

    void begin() {
        //m_encoder = bgfx::begin(true);
    }

    void end() {
        //bgfx::end(m_encoder);
        //m_encoder = nullptr;
    }

    void loadPrograms() {
        etes_erosion_program = bgfx::createProgram(loadShader("cs_etes_erosion"), true);
        etes_erosion_init = bgfx::createProgram(loadShader("cs_etes_init"), true);
    }

    void updateTextures() {
        bgfx::updateTexture2D(elevation_data,
            0, 0, 0, 0,
            (uint16_t)data->getWidth(),
            (uint16_t)data->getHeight(),
            bgfx::makeRef(data->get()->m_data, data->getSize()));
        // bgfx::updateTexture2D(ground_data,
        //     0, 0, 0, 0,
        //     (uint16_t)data->getWidth(),
        //     (uint16_t)data->getHeight(),
        //     bgfx::makeRef(data->get()->m_data, data->getSize()));
    }

    void loadTextures() {
        if (!bgfx::isValid(elevation_data)) {
            elevation_data = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);
                // bgfx::makeRef(data->get()->m_data, data->getSize())};
        }

        if (!bgfx::isValid(ground_data)) {
            ground_data = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_COMPUTE_WRITE);
                // bgfx::makeRef(data->get()->m_data, data->getSize())};
        }

        if (!bgfx::isValid(cell_data_a)) {
            cell_data_a = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::RGBA32U, BGFX_TEXTURE_COMPUTE_WRITE);
        }

        if (!bgfx::isValid(cell_data_b)) {
            cell_data_b = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::RGBA32U, BGFX_TEXTURE_COMPUTE_WRITE);
        }

        if (!bgfx::isValid(rand_data)) {
            std::vector<float> randdat;
            randdat.resize(512*512);
            for (auto& d : randdat) {
                d = rand() / (float)RAND_MAX;
            }
            rand_data = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::R32F, 0, bgfx::copy(randdat.data(), randdat.size() * sizeof(float)));
        }

        if (!bgfx::isValid(computeListBufferA)) {
            computeListBufferA = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::R32U, BGFX_TEXTURE_COMPUTE_WRITE);
        }

        if (!bgfx::isValid(computeListBufferB)) {
            computeListBufferB = bgfx::createTexture2D(512, 512, false, 1, bgfx::TextureFormat::R32U, BGFX_TEXTURE_COMPUTE_WRITE);
        }
    }

    void loadBuffers() {
        if (!bgfx::isValid(computeEventBufferA)) {
            computeEventBufferA = bgfx::createDynamicVertexBuffer(512*512, computeVertexLayoutEvent, BGFX_BUFFER_COMPUTE_READ_WRITE);
        }
        if (!bgfx::isValid(computeEventBufferB)) {
            computeEventBufferB = bgfx::createDynamicVertexBuffer(512*512, computeVertexLayoutEvent, BGFX_BUFFER_COMPUTE_READ_WRITE);
        }
    }

    void downloadTextures() {
        if (bgfx::isValid(elevation_data)) {
            bgfx::readTexture(elevation_data, data->get()->m_data);
            //void* d = bgfx::getDirectAccessPtr(data_b.getHandle());
        }
    }

    // void copyTerrain(const std::shared_ptr<graphics::Texture> terr, float s) {
    //     w = terr->getWidth();
    //     h = terr->getHeight();
    //     bimg::ImageContainer* image = bimg::imageAlloc(getAllocator(), bimg::TextureFormat::RGBA32F, w, h, 0, 1, false, false);
    //     float* dmap = (float*)image->m_data;
    //     // copy data into local image
    //     for (int x = 0; x < w; ++x) {
    //         for (int y = 0; y < h; ++y) {
    //             float h = s * terr->at(x, y);
    //             dmap[0 + 4 * (x + w * y)] = h; // red channel is height
    //             dmap[1 + 4 * (x + w * y)] = 0;
    //             dmap[2 + 4 * (x + w * y)] = 0;
    //             dmap[3 + 4 * (x + w * y)] = 0;
    //         }
    //     }
    //     data = std::make_shared<resource::ImageData>(image);
    // }

    // bool copyTerrainTo(std::shared_ptr<graphics::Texture> terr, float s, const ETESModelParameters& parameters) {
    //     if (data) {
    //         float* dmap = (float*)data->get()->m_data;
    //         for (int x = 0; x < w; ++x) {
    //             for (int y = 0; y < h; ++y) {
    //                 float h =   dmap[0 + 4 * (x + w * y)] + 
    //                             dmap[1 + 4 * (x + w * y)] + 
    //                             dmap[2 + 4 * (x + w * y)] + 
    //                             dmap[3 + 4 * (x + w * y)]; // red channel is height
    //                 h = h / s;
    //                 terr->at(x, y) = h;
    //             }
    //         }
    //         return true;
    //     }
    //     return false;
    // }
};

EcosystemTerrainErosionSimulationGPU::EcosystemTerrainErosionSimulationGPU(std::shared_ptr<Terrain> target)
    : Erosion{"EcosystemTerrainErosionSimulationGPU", std::move(target)}, m_etesgpu{std::make_unique<ETESGPUImpl>()}
{
    params.toParameterSet(parameters);

    m_etesgpu->loadPrograms();
    
}

void EcosystemTerrainErosionSimulationGPU::init() {
    if (!m_isRunning) {
        params.fromParameterSet(parameters);
        m_etesgpu->uniforms.applyParameter(params);

        m_etesgpu->loadTextures();
        m_etesgpu->loadBuffers();

        m_itercounter = 0;
    }
}

void EcosystemTerrainErosionSimulationGPU::startErosionTask() {
    // if (!m_isRunning) {
    //     m_etesgpu->copyTerrain(terr, 500);
    //     m_etesgpu->updateTextures();
    //     m_isRunning = true;
    // }
}

float EcosystemTerrainErosionSimulationGPU::getProgress() const {
    return m_itercounter / (params.iterations * 25.0f);
}

bool EcosystemTerrainErosionSimulationGPU::isRunning() const {
    return m_isRunning;
}

void EcosystemTerrainErosionSimulationGPU::stopErosionTask() {
    // TODO
}

void EcosystemTerrainErosionSimulationGPU::update() {
    if (m_isRunning) {
        run_erosion();
    }
}

void EcosystemTerrainErosionSimulationGPU::read() {
    m_etesgpu->downloadTextures();
}

void EcosystemTerrainErosionSimulationGPU::run_erosion() {
    if (m_isRunning) {
        if (m_itercounter % 25 == 0) {
            // reset events
            if (m_etesgpu->A_B) {
                bgfx::setImage(2, m_etesgpu->computeListBufferA, 0,    bgfx::Access::Write, bgfx::TextureFormat::R32U);
            } else {
                bgfx::setImage(2, m_etesgpu->computeListBufferB, 0,    bgfx::Access::Write, bgfx::TextureFormat::R32U);
            }
            //m_etesgpu->begin();
            bgfx::setImage(0, m_etesgpu->cell_data_a, 0,          bgfx::Access::Write, bgfx::TextureFormat::RGBA32U);
            bgfx::setBuffer(1, m_etesgpu->computeEventBufferA,   bgfx::Access::ReadWrite);
            m_etesgpu->uniforms.submit();
            bgfx::dispatch(3, m_etesgpu->etes_erosion_init, 512, 512);
        } else {
            //bgfx::touch(3);
            bgfx::setImage(0, m_etesgpu->elevation_data, 0,     bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);
            bgfx::setImage(1, m_etesgpu->ground_data, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32F);
            
            if (m_etesgpu->A_B) {
                bgfx::setImage(2, m_etesgpu->cell_data_a, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32U);
                bgfx::setImage(3, m_etesgpu->cell_data_b, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32U);
            } else {
                bgfx::setImage(2, m_etesgpu->cell_data_b, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32U);
                bgfx::setImage(3, m_etesgpu->cell_data_a, 0,        bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32U);
            }
            
            bgfx::setImage(4, m_etesgpu->rand_data, 0,          bgfx::Access::Read, bgfx::TextureFormat::RGBA32F);
            bgfx::setBuffer(7, m_etesgpu->computeEventBufferA,   bgfx::Access::ReadWrite);
            bgfx::setBuffer(8, m_etesgpu->computeEventBufferB,  bgfx::Access::ReadWrite);
            
            if (m_etesgpu->A_B) {
                bgfx::setImage(5, m_etesgpu->computeListBufferA, 0,    bgfx::Access::Read, bgfx::TextureFormat::R32I);
                bgfx::setImage(6, m_etesgpu->computeListBufferB, 0,   bgfx::Access::Write, bgfx::TextureFormat::R32I);
            } else {
                bgfx::setImage(5, m_etesgpu->computeListBufferB, 0,   bgfx::Access::Read, bgfx::TextureFormat::R32I);
                bgfx::setImage(6, m_etesgpu->computeListBufferA, 0,   bgfx::Access::Write, bgfx::TextureFormat::R32I);
            }
            m_etesgpu->uniforms.rand_offset[0] = rand() / (float)RAND_MAX;
            m_etesgpu->uniforms.rand_offset[1] = rand() / (float)RAND_MAX;
            m_etesgpu->uniforms.submit();
            bgfx::dispatch(3, m_etesgpu->etes_erosion_program, 512, 512);

            m_etesgpu->A_B = !m_etesgpu->A_B;
        }
        
        m_itercounter++;
        if (m_itercounter > params.iterations * 25) {
            m_isRunning = false;
        }
    }
}

}
