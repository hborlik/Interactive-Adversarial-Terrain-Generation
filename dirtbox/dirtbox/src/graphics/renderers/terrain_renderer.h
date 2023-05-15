/**
 
 * @brief 
 * @version 0.1
 * @date 2021-03-02
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_TERRAIN_RENDERER_H
#define DIRTBOX_TERRAIN_RENDERER_H

#include <string>

#include <bgfx/bgfx.h>
#include <graphics/texture.h>
#include <terrain/terrain.h>

namespace dirtbox {

constexpr int32_t KNumVec4 = 2;

struct ComputeTessUniforms
{
    void init()
    {
        u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, KNumVec4);

        cull = 1;
        freeze = 0;
        gpuSubd = 3;

        dmapFactor = 0.192f;
    }

    void submit()
    {
        bgfx::setUniform(u_params, params, KNumVec4);
    }

    void destroy()
    {
        bgfx::destroy(u_params);
    }

    union
    {
        struct
        {
            float dmapFactor;
            float lodFactor;
            float cull;
            float freeze;

            float gpuSubd;
            float padding0;
            float padding1;
            float padding2;
        };

        float params[KNumVec4 * 4];
    };

    bgfx::UniformHandle u_params;
};

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    void draw();

    void init();

    void setCameraInfo(const float* viewMtx, int width, int height, float fov);

    enum class ShadingMode {
        Normal,
        Diffuse
    };
    
    void setShadingMode(ShadingMode mode) {m_shading = (int)mode;}
    ShadingMode getShadingMode() const {return (ShadingMode)m_shading;}

    void setTerrain(std::shared_ptr<terrain::Terrain> terrain);

    ComputeTessUniforms& getUniforms() {return m_uniforms;}

    float getPixelLengthTarget() const {return m_primitivePixelLengthTarget;}
    void setPixelLengthTarget(float value) {m_primitivePixelLengthTarget = value;}

    void forceRestart() {m_restart = true;}

private:

    void updateUniforms();

    void loadBuffers();
    void loadSubdivisionBuffers();
    void loadGeometryBuffers();
    void loadInstancedGeometryBuffers();

    void createAtomicCounters();

    void loadPrograms();
    void createTextures();


    enum
    {
        PROGRAM_TERRAIN_NORMAL,
        PROGRAM_TERRAIN,
        //PROGRAM_TEXTURED,

        SHADING_COUNT
    };

    enum
    {
        BUFFER_SUBD
    };

    enum
    {
        PROGRAM_SUBD_CS_LOD,
        PROGRAM_UPDATE_INDIRECT,
        PROGRAM_INIT_INDIRECT,
        PROGRAM_UPDATE_DRAW,

        PROGRAM_COUNT
    };

    enum
    {
        TERRAIN_DMAP_SAMPLER,

        SAMPLER_COUNT
    };

    ComputeTessUniforms m_uniforms;

    bgfx::ProgramHandle m_programsCompute[PROGRAM_COUNT];
    bgfx::ProgramHandle m_programsDraw[SHADING_COUNT];

    std::shared_ptr<terrain::Terrain> m_dmap;
    bgfx::UniformHandle m_samplers[SAMPLER_COUNT];

    bgfx::DynamicIndexBufferHandle m_bufferSubd[2];
    bgfx::DynamicIndexBufferHandle m_bufferCulledSubd;

    bgfx::DynamicIndexBufferHandle m_bufferCounter;

    bgfx::IndexBufferHandle m_geometryIndices;
    bgfx::VertexBufferHandle m_geometryVertices;
    bgfx::VertexLayout m_geometryLayout;

    bgfx::IndexBufferHandle m_instancedGeometryIndices;
    bgfx::VertexBufferHandle m_instancedGeometryVertices;
    bgfx::VertexLayout m_instancedGeometryLayout;

    bgfx::IndirectBufferHandle m_dispatchIndirect;

    // camera info
    float m_viewMtx[16];
    float m_projMtx[16];
    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_instancedMeshVertexCount;
    uint32_t m_instancedMeshPrimitiveCount;

    int m_computeThreadCount;
    int m_shading;
    int m_gpuSubd;
    int m_pingPong;

    float m_primitivePixelLengthTarget;
    float m_fovy;

    uint16_t m_terrain_width;
    uint16_t m_terrain_height;

    bool m_restart = true;
};

}

#endif // DIRTBOX_TERRAIN_RENDERER_H