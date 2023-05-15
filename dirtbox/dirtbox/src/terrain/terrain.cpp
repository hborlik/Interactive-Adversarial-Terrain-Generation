#include <terrain/terrain.h>
#include <terrain/terrain_manager.h>
#include <terrain/erosion_model2.h>
#include <resource/resource_manager.h>
#include <core/core.h>


namespace dirtbox::terrain {

Terrain::Terrain(const vec2u& size) :
    m_terrain{std::make_unique<graphics::Texture>(size.x(), size.y(), bgfx::TextureFormat::RGBA32F, 1, BGFX_TEXTURE_READ_BACK)} {
}

bool Terrain::loadTerrain(const std::string& filename)
{
    auto inimg = resource::ResourceManager::Load<resource::ImageData>(filename, bgfx::TextureFormat::R32F);
    if (inimg) {
        return setTerrainData(*inimg);
    }
    return false;
}

void Terrain::saveTerrain(const std::string& filename) {
    Core& core = Core::Get();
    std::shared_future fut{m_terrain->getImageData()};
    core.getTaskManager().add_task(
        TaskManager::TaskFnType::create([filename, f = std::move(fut)]() mutable -> int { // TODO figure out how to maintain a copy of the lambda object!
            return f.get().saveImage(filename) ? 0 : -1;
    }));
}

bool Terrain::setTerrainData(const resource::ImageData& image) {
    auto img = image.getAsFormat(bgfx::TextureFormat::RGBA32F);
    for (int x = 0; x < img.getWidth(); ++x)
        for (int y = 0; y < img.getHeight(); ++y) {
            float* dptr = static_cast<float*>(img.get()->m_data);
            dptr[1 + 4 * x + 4 * (y * img.getHeight())] = 0;
            dptr[2 + 4 * x + 4 * (y * img.getHeight())] = 0;
            dptr[3 + 4 * x + 4 * (y * img.getHeight())] = 0;
        }
    m_terrain->resize({img.getWidth(), img.getHeight()});
    return m_terrain->setImageData(img, 0, 0, img.getWidth(), img.getHeight());
}

vec2u Terrain::getSize() const {
    return m_terrain->getDim().xy();
}

TerrainManager::TerrainManager(const vec2u& size) :
    m_terrain{std::make_shared<Terrain>(size)}
{

}

std::unique_ptr<Erosion> TerrainManager::createErosion(const std::string& name) {
    if (name == "Erosion2SimulationGPU")
        return std::make_unique<Erosion2SimulationGPU>(m_terrain);
    return {};
}

}