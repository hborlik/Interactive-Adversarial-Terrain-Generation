/**
 * @file terrain.h
 * @brief 
 * @version 0.1
 * @date 2021-04-29
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_TERRAIN_H
#define DIRTBOX_TERRAIN_H

#include <memory>
#include <string>

#include <graphics/texture.h>
#include <terrain/erosion.h>

namespace dirtbox::terrain {

class Terrain {
public:
    Terrain(const vec2u& size);

    bool loadTerrain(const std::string& filename);
    void saveTerrain(const std::string& filename);
    bool setTerrainData(const resource::ImageData& image);
    vec2u getSize() const;
    const graphics::Texture& getTerrainTexture() const {return *m_terrain;}

private:
    std::shared_ptr<graphics::Texture> m_terrain;
};

}

#endif // DIRTBOX_TERRAIN_H