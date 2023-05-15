/**
 
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-03-02
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_EROSION_H
#define DIRTBOX_EROSION_H

#include <memory>
#include <functional>

#include <graphics/texture.h>

#include <util/parameter.h>

namespace dirtbox::terrain {

class Terrain;

class Erosion {
public:
    Erosion(const std::string& Name, std::shared_ptr<Terrain> target) : Name{Name}, target{std::move(target)} {}
    virtual ~Erosion() {}

    util::ParameterList<float> getParams() const {return parameters.getParams();}
    bool setParam(const std::string& key, float value) {return parameters.setParam(key, value);}
    float getParam(const std::string& name) const {return parameters.getParam(name);}

    virtual void startErosionTask() = 0;
    virtual void stopErosionTask() = 0;

    virtual float getProgress() const = 0;
    virtual bool isRunning() const = 0;
    virtual void update() = 0;
    
    const std::string Name;

protected:
    util::ParameterCollection<float> parameters;
    std::shared_ptr<Terrain> target;
};

}

#endif // DIRTBOX_EROSION_H