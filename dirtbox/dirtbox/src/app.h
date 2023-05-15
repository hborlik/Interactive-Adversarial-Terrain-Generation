/**
 
 * @author Hunter Borlik
 * @brief Initialization and startup
 *  based on bgfx examples by Branimir Karadzic
 * 
 * @date 2021-01-15
 * 
 */
#pragma once
#ifndef DIRTBOX_APP_H_HEADER_GUARD
#define DIRTBOX_APP_H_HEADER_GUARD

#include <bgfx/bgfx.h>
#include <terrain/terrain.h>
#include <resource/resource_manager.h>
#include <terrain/terrain_manager.h>
#include <graphics/renderers/terrain_renderer.h>

namespace dirtbox
{

int runApp(int argc, char *args[], uint32_t defaultWidth, uint32_t defaultHeight, const float scale);

terrain::TerrainManager& GetTerrainManager();
dirtbox::TerrainRenderer& GetTerrainRenderer();
resource::ResourceManager& GetResourceManager();

} // namespace dirtbox

#endif // DIRTBOX_APP_H_HEADER_GUARD