/**
 
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_GENERATOR_H
#define DIRTBOX_GENERATOR_H

#include <string>
#include <map>

#include <graphics/texture.h>
#include <util/parameter.h>
#include <terrain/terrain.h>

namespace dirtbox::terrain {

class Generator {
public:
    Generator(vec2u outputSize) : outputSize{outputSize} {}
    virtual ~Generator() {}
    virtual void Generate(terrain::Terrain& t) const = 0;

    util::ParameterList<float> getParams() const {return parameters.getParams();}
    bool setParam(const std::string& key, float value) {return parameters.setParam(key, value);}
    float getParam(const std::string& name) const {return parameters.getParam(name);}

    vec2u getOutputSize() const {return outputSize;}

protected:

    util::ParameterCollection<float> parameters;
    const vec2u outputSize;
};

class HttpGanGenerator : public Generator {
public:
    static const vec2u TargetSize;
    HttpGanGenerator() : 
        Generator{TargetSize},
        inputImage{resource::ImageData::CreateSolidImage(TargetSize, bgfx::TextureFormat::RGBA8, 0)}
    {}
    virtual ~HttpGanGenerator() {}
    void Generate(terrain::Terrain& t) const override;

    resource::ImageData& GetInputImage() {return inputImage;}

    int getPort() const {return hostPort;}
    void setPort(int port) {hostPort = port;}

    const std::string& getHostname() const {return hostName;}
    void setHostname(const std::string& hostname) {hostName = hostname;}

protected:
    resource::ImageData inputImage;
    int hostPort = 8080;
    std::string hostName = "localhost";
};

}

#endif // DIRTBOX_GENERATOR_H