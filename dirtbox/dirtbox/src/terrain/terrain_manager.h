/**
 * @brief 
 * @version 0.1
 * @date 2021-05-12
 * 
 */
#pragma once
#ifndef DIRTBOX_TERRAIN_MANAGER_H
#define DIRTBOX_TERRAIN_MANAGER_H

#include <memory>
#include <terrain/erosion.h>

namespace dirtbox::terrain {

class TerrainManager {
public:
    TerrainManager(const vec2u& size);

    std::shared_ptr<terrain::Terrain> getTerrain() {return m_terrain;}
    std::unique_ptr<Erosion> createErosion(const std::string& name);

private:
    std::shared_ptr<terrain::Terrain> m_terrain;
};

}

#endif // DIRTBOX_TERRAIN_MANAGER_H