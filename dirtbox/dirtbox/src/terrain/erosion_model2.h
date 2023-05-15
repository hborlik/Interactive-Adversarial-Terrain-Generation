/**
 
 * @brief GPU erosion based on 
 * @version 0.1
 * @date 2021-04-14
 * 
 * 
 */
#pragma once
#ifndef STAVA_EROSION_H
#define STAVA_EROSION_H

#include <terrain/erosion.h>
#include <bgfx/bgfx.h>

namespace dirtbox::terrain {

class Erosion2SimulationGPU : public Erosion {
public:
    Erosion2SimulationGPU(std::shared_ptr<Terrain> target);

    void startErosionTask() override;
    void stopErosionTask() override;

    float getProgress() const override;
    bool isRunning() const override;
    void update() override;

    bgfx::TextureHandle getDisplayHeightmap();

private:
    void run_erosion();

    bool m_isRunning = false;
    uint32_t m_itercounter = 0;
    std::shared_ptr<class Erosion2GPUImpl> m_gpu;
};

} // namespace dirtbox::terrain

#endif // STAVA_EROSION_H