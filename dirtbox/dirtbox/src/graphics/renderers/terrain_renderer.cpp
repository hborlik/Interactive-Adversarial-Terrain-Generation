#include <graphics/renderers/terrain_renderer.h>

#include <limits>

#include <bx/bx.h>
#include <bx/math.h>
#include <util/box_utils.h>

namespace dirtbox {

static const float s_verticesL0[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
};

static const uint32_t s_indexesL0[] = { 0u, 1u, 2u };

static const float s_verticesL1[] =
{
    0.0f, 1.0f,
    0.5f, 0.5f,
    0.0f, 0.5f,
    0.0f, 0.0f,
    0.5f, 0.0f,
    1.0f, 0.0f,
};

static const uint32_t s_indexesL1[] =
{
    1u, 0u, 2u,
    1u, 2u, 3u,
    1u, 3u, 4u,
    1u, 4u, 5u,
};

static const float s_verticesL2[] =
{
    0.25f, 0.75f,
    0.0f,  1.0f,
    0.0f,  0.75f,
    0.0f,  0.5f,
    0.25f, 0.5f,
    0.5f,  0.5f,

    0.25f, 0.25f,
    0.0f,  0.25f,
    0.0f,  0.0f,
    0.25f, 0.0f,
    0.5f,  0.0f,
    0.5f,  0.25f,
    0.75f, 0.25f,
    0.75f, 0.0f,
    1.0f,  0.0f,
};

static const uint32_t s_indexesL2[] =
{
    0u, 1u, 2u,
    0u, 2u, 3u,
    0u, 3u, 4u,
    0u, 4u, 5u,

    6u, 5u, 4u,
    6u, 4u, 3u,
    6u, 3u, 7u,
    6u, 7u, 8u,

    6u, 8u, 9u,
    6u, 9u, 10u,
    6u, 10u, 11u,
    6u, 11u, 5u,

    12u, 5u, 11u,
    12u, 11u, 10u,
    12u, 10u, 13u,
    12u, 13u, 14u,
};

static const float s_verticesL3[] =
{
    0.25f*0.5f, 0.75f*0.5f + 0.5f,
    0.0f*0.5f, 1.0f*0.5f + 0.5f,
    0.0f*0.5f, 0.75f*0.5f + 0.5f,
    0.0f*0.5f , 0.5f*0.5f + 0.5f,
    0.25f*0.5f, 0.5f*0.5f + 0.5f,
    0.5f*0.5f, 0.5f*0.5f + 0.5f,
    0.25f*0.5f, 0.25f*0.5f + 0.5f,
    0.0f*0.5f, 0.25f*0.5f + 0.5f,
    0.0f*0.5f, 0.0f*0.5f + 0.5f,
    0.25f*0.5f, 0.0f*0.5f + 0.5f,
    0.5f*0.5f, 0.0f*0.5f + 0.5f,
    0.5f*0.5f, 0.25f*0.5f + 0.5f,
    0.75f*0.5f, 0.25f*0.5f + 0.5f,
    0.75f*0.5f, 0.0f*0.5f + 0.5f,
    1.0f*0.5f, 0.0f*0.5f + 0.5f,        //14

    0.375f, 0.375f,
    0.25f, 0.375f,
    0.25f, 0.25f,
    0.375f, 0.25f,
    0.5f, 0.25f,
    0.5f, 0.375f,    //20

    0.125f, 0.375f,
    0.0f, 0.375f,
    0.0f, 0.25f,
    0.125f, 0.25f,    //24

    0.125f, 0.125f,
    0.0f, 0.125f,
    0.0f, 0.0f,
    0.125f, 0.0f,
    0.25f, 0.0f,
    0.25f, 0.125f,    //30

    0.375f, 0.125f,
    0.375f, 0.0f,
    0.5f, 0.0f,
    0.5f, 0.125f,    //34

    0.625f, 0.375f,
    0.625f, 0.25f,
    0.75f, 0.25f,    //37

    0.625f, 0.125f,
    0.625f, 0.0f,
    0.75f, 0.0f,
    0.75f, 0.125f,    //41

    0.875f, 0.125f,
    0.875f, 0.0f,
    1.0f, 0.0f,    //44
};

static const uint32_t s_indexesL3[] =
{
    0u, 1u, 2u,
    0u, 2u, 3u,
    0u, 3u, 4u,
    0u, 4u, 5u,

    6u, 5u, 4u,
    6u, 4u, 3u,
    6u, 3u, 7u,
    6u, 7u, 8u,

    6u, 8u, 9u,
    6u, 9u, 10u,
    6u, 10u, 11u,
    6u, 11u, 5u,

    12u, 5u, 11u,
    12u, 11u, 10u,
    12u, 10u, 13u,
    12u, 13u, 14u,        //End fo first big triangle

    15u, 14u, 13u,
    15u, 13u, 10u,
    15u, 10u, 16u,
    15u, 16u, 17u,
    15u, 17u, 18u,
    15u, 18u, 19u,
    15u, 19u, 20u,
    15u, 20u, 14u,

    21u, 10u, 9u,
    21u, 9u, 8u,
    21u, 8u, 22u,
    21u, 22u, 23u,
    21u, 23u, 24u,
    21u, 24u, 17u,
    21u, 17u, 16u,
    21u, 16u, 10u,

    25u, 17u, 24u,
    25u, 24u, 23u,
    25u, 23u, 26u,
    25u, 26u, 27u,
    25u, 27u, 28u,
    25u, 28u, 29u,
    25u, 29u, 30u,
    25u, 30u, 17u,

    31u, 19u, 18u,
    31u, 18u, 17u,
    31u, 17u, 30u,
    31u, 30u, 29u,
    31u, 29u, 32u,
    31u, 32u, 33u,
    31u, 33u, 34u,
    31u, 34u, 19u,

    35u, 14u, 20u,
    35u, 20u, 19u,
    35u, 19u, 36u,
    35u, 36u, 37u,

    38u, 37u, 36u,
    38u, 36u, 19u,
    38u, 19u, 34u,
    38u, 34u, 33u,
    38u, 33u, 39u,
    38u, 39u, 40u,
    38u, 40u, 41u,
    38u, 41u, 37u,

    42u, 37u, 41u,
    42u, 41u, 40u,
    42u, 40u, 43u,
    42u, 43u, 44u,
};

TerrainRenderer::TerrainRenderer() {
    for (uint32_t i = 0; i < PROGRAM_COUNT; ++i)
    {
        m_programsCompute[i].idx = bgfx::kInvalidHandle;
    }

    for (uint32_t i = 0; i < SHADING_COUNT; ++i)
    {
        m_programsDraw[i].idx = bgfx::kInvalidHandle;
    }

    for (uint32_t i = 0; i < SAMPLER_COUNT; ++i)
    {
        m_samplers[i].idx = bgfx::kInvalidHandle;
    }
}

TerrainRenderer::~TerrainRenderer() {
    m_uniforms.destroy();

    bgfx::destroy(m_bufferCounter);
    bgfx::destroy(m_bufferCulledSubd);
    bgfx::destroy(m_bufferSubd[0]);
    bgfx::destroy(m_bufferSubd[1]);
    bgfx::destroy(m_dispatchIndirect);
    bgfx::destroy(m_geometryIndices);
    bgfx::destroy(m_geometryVertices);
    bgfx::destroy(m_instancedGeometryIndices);
    bgfx::destroy(m_instancedGeometryVertices);

    for (uint32_t i = 0; i < PROGRAM_COUNT; ++i)
    {
        bgfx::destroy(m_programsCompute[i]);
    }

    for (uint32_t i = 0; i < SHADING_COUNT; ++i)
    {
        bgfx::destroy(m_programsDraw[i]);
    }

    for (uint32_t i = 0; i < SAMPLER_COUNT; ++i)
    {
        bgfx::destroy(m_samplers[i]);
    }
}

void TerrainRenderer::draw() {
    bgfx::touch(0);
    bgfx::touch(1);
    if (m_dmap) {

        updateUniforms();

        float model[16];

        bx::mtxRotateX(model, bx::toRad(90) );

        bx::mtxProj(m_projMtx, m_fovy, float(m_width) / float(m_height), 0.0001f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

        // Set view 0
        bgfx::setViewTransform(0, m_viewMtx, m_projMtx);

        // Set view 1
        bgfx::setViewRect(1, 0, 0, uint16_t(m_width), uint16_t(m_height) );
        bgfx::setViewTransform(1, m_viewMtx, m_projMtx);

        m_uniforms.submit();

        // update the subd buffers
        if (m_restart)
        {
            m_pingPong = 1;

            bgfx::destroy(m_instancedGeometryVertices);
            bgfx::destroy(m_instancedGeometryIndices);

            bgfx::destroy(m_bufferSubd[BUFFER_SUBD]);
            bgfx::destroy(m_bufferSubd[BUFFER_SUBD + 1]);
            bgfx::destroy(m_bufferCulledSubd);

            loadInstancedGeometryBuffers();
            loadSubdivisionBuffers();

            //init indirect
            bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
            bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
            bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
            bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
            bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::ReadWrite);
            bgfx::dispatch(0, m_programsCompute[PROGRAM_INIT_INDIRECT], 1, 1, 1);


            m_restart = false;
        }
        else
        {
            // update batch
            bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
            bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
            bgfx::dispatch(0, m_programsCompute[PROGRAM_UPDATE_INDIRECT], 1, 1, 1);
        }

        bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
        bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
        bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
        bgfx::setBuffer(6, m_geometryVertices, bgfx::Access::Read);
        bgfx::setBuffer(7, m_geometryIndices, bgfx::Access::Read);
        bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::Read);
        bgfx::setTransform(model);

        bgfx::setTexture(0, m_samplers[TERRAIN_DMAP_SAMPLER], m_dmap->getTerrainTexture().getHandle(), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);


        m_uniforms.submit();

        // update the subd buffer
        bgfx::dispatch(0, m_programsCompute[PROGRAM_SUBD_CS_LOD], m_dispatchIndirect, 1);

        // update draw
        bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
        bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);

        m_uniforms.submit();

        bgfx::dispatch(1, m_programsCompute[PROGRAM_UPDATE_DRAW], 1, 1, 1);

        // render the terrain
        bgfx::setTexture(0, m_samplers[TERRAIN_DMAP_SAMPLER], m_dmap->getTerrainTexture().getHandle(), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

        bgfx::setTransform(model);
        bgfx::setVertexBuffer(0, m_instancedGeometryVertices);
        bgfx::setIndexBuffer(m_instancedGeometryIndices);
        bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::Read);
        bgfx::setBuffer(3, m_geometryVertices, bgfx::Access::Read);
        bgfx::setBuffer(4, m_geometryIndices, bgfx::Access::Read);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

        m_uniforms.submit();

        bgfx::submit(1, m_programsDraw[m_shading], m_dispatchIndirect);

        m_pingPong = 1 - m_pingPong;
    }
}

void TerrainRenderer::init() {
    m_uniforms.freeze = false;
    m_uniforms.cull = true;

    m_computeThreadCount = 5;
    m_shading = PROGRAM_TERRAIN;
    m_primitivePixelLengthTarget = 7.0f;
    m_fovy = 60.0f;
    m_pingPong = 0;
    m_restart = true;

    loadPrograms();
    loadBuffers();

    createAtomicCounters();

    m_dispatchIndirect = bgfx::createIndirectBuffer(2);
}

void TerrainRenderer::setCameraInfo(const float* viewMtx, int width, int height, float fov) {
    bx::memCopy(m_viewMtx, viewMtx, sizeof(float) * 16);
    m_width = width;
    m_height = height;
    m_fovy = fov;
}

void TerrainRenderer::updateUniforms()
{
    float lodFactor = 2.0f * bx::tan(bx::toRad(m_fovy) / 2.0f)
        / m_width * (1 << (int)m_uniforms.gpuSubd)
        * m_primitivePixelLengthTarget;

    m_uniforms.lodFactor = lodFactor;
}

/**
 * Load the Geometry Buffer
 *
 * This procedure loads the scene geometry into an index and
 * vertex buffer. Here, we only load 2 triangles to define the
 * terrain.
 **/
void TerrainRenderer::loadGeometryBuffers()
{
    const float vertices[] =
    {
        -1.0f, -1.0f, 0.0f, 1.0f,
        +1.0f, -1.0f, 0.0f, 1.0f,
        +1.0f, +1.0f, 0.0f, 1.0f,
        -1.0f, +1.0f, 0.0f, 1.0f,
    };

    const uint32_t indices[] = { 0, 1, 3, 2, 3, 1 };

    m_geometryLayout.begin().add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float).end();

    m_geometryVertices = bgfx::createVertexBuffer(
            bgfx::copy(vertices, sizeof(vertices) )
        , m_geometryLayout
        , BGFX_BUFFER_COMPUTE_READ
        );
    m_geometryIndices = bgfx::createIndexBuffer(
            bgfx::copy(indices, sizeof(indices) )
        , BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32
        );
}

void TerrainRenderer::loadSubdivisionBuffers()
{
    const uint32_t bufferCapacity = 1 << 27;

    m_bufferSubd[BUFFER_SUBD] = bgfx::createDynamicIndexBuffer(
            bufferCapacity
        , BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
        );

    m_bufferSubd[BUFFER_SUBD + 1] = bgfx::createDynamicIndexBuffer(
            bufferCapacity
        , BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
        );

    m_bufferCulledSubd = bgfx::createDynamicIndexBuffer(
            bufferCapacity
        , BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
        );
}

/**
 * Load All Buffers
 *
 */
void TerrainRenderer::loadBuffers()
{
    loadSubdivisionBuffers();
    loadGeometryBuffers();
    loadInstancedGeometryBuffers();
}

/**
* This will be used to instantiate a triangle grid for each subdivision
* key present in the subd buffer.
*/
void TerrainRenderer::loadInstancedGeometryBuffers()
{
    const float* vertices;
    const uint32_t* indexes;

    switch (int32_t(m_uniforms.gpuSubd) )
    {
    case 0:
        m_instancedMeshVertexCount = 3;
        m_instancedMeshPrimitiveCount = 1;
        vertices = s_verticesL0;
        indexes  = s_indexesL0;
        break;

    case 1:
        m_instancedMeshVertexCount = 6;
        m_instancedMeshPrimitiveCount = 4;
        vertices = s_verticesL1;
        indexes  = s_indexesL1;
        break;

    case 2:
        m_instancedMeshVertexCount = 15;
        m_instancedMeshPrimitiveCount = 16;
        vertices = s_verticesL2;
        indexes  = s_indexesL2;
        break;

    default:
        m_instancedMeshVertexCount = 45;
        m_instancedMeshPrimitiveCount = 64;
        vertices = s_verticesL3;
        indexes  = s_indexesL3;
        break;
    }

    m_instancedGeometryLayout
        .begin()
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    m_instancedGeometryVertices = bgfx::createVertexBuffer(
            bgfx::makeRef(vertices, sizeof(float) * 2 * m_instancedMeshVertexCount)
        , m_instancedGeometryLayout
        );

    m_instancedGeometryIndices  = bgfx::createIndexBuffer(
            bgfx::makeRef(indexes, sizeof(uint32_t) * m_instancedMeshPrimitiveCount * 3)
        , BGFX_BUFFER_INDEX32
        );
}

void TerrainRenderer::createAtomicCounters()
{
    m_bufferCounter = bgfx::createDynamicIndexBuffer(3, BGFX_BUFFER_INDEX32 | BGFX_BUFFER_COMPUTE_READ_WRITE);
}

/**
* Load the Terrain Program
*
* This program renders an adaptive terrain using the implicit subdivision
* technique discribed in GPU Zen 2.
**/
void TerrainRenderer::loadPrograms()
{
    m_samplers[TERRAIN_DMAP_SAMPLER] = bgfx::createUniform("u_DmapSampler", bgfx::UniformType::Sampler);

    m_uniforms.init();

    m_programsDraw[PROGRAM_TERRAIN] = loadProgram("vs_terrain_render", "fs_terrain_render");
    m_programsDraw[PROGRAM_TERRAIN_NORMAL] = loadProgram("vs_terrain_render", "fs_terrain_render_normal");

    m_programsCompute[PROGRAM_SUBD_CS_LOD] = bgfx::createProgram(loadShader("cs_terrain_lod"), true);
    m_programsCompute[PROGRAM_UPDATE_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_update_indirect"), true);
    m_programsCompute[PROGRAM_UPDATE_DRAW] = bgfx::createProgram(loadShader("cs_terrain_update_draw"), true);
    m_programsCompute[PROGRAM_INIT_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_init"), true);
}

void TerrainRenderer::setTerrain(std::shared_ptr<terrain::Terrain> direct) {
    m_dmap = direct;
    m_terrain_width = m_dmap->getSize().x();
    m_terrain_height = m_dmap->getSize().y();
}

}