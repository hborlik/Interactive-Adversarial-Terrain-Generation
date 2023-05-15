/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-15
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_TEXTURE_H
#define DIRTBOX_TEXTURE_H

#include <stdexcept>
#include <future>

#include <resource/image.h>
#include <bgfx/bgfx.h>

namespace dirtbox::graphics {

class Texture {
public:
    Texture() {}

    Texture(uint16_t width, uint16_t height, bgfx::TextureFormat::Enum format, uint16_t num_layers = 1, uint64_t flags = 0ul, const bgfx::Memory* mem = nullptr) :
        flags{flags} {
        if (!bgfx::isTextureValid(0, false, num_layers, format, flags))
            throw std::runtime_error{"Bad Texture"};
        bgfx::calcTextureSize(ti, width, height, 1, false, num_layers > 1, num_layers, format);
        m_texture = bgfx::createTexture2D(width, height, num_layers > 1, num_layers, format, flags, mem);
    }

    Texture (const resource::ImageData& image, uint64_t flags = 0ul) :
        flags{flags} {
        bgfx::calcTextureSize(ti, image.getWidth(), image.getHeight(), 1, false, image.getNumLayers() > 1, image.getNumLayers(), (bgfx::TextureFormat::Enum)(image.getFormat()));
        auto* mem = bgfx::copy(image.get()->m_data, image.getSize());
        m_texture = bgfx::createTexture2D(ti.width, ti.height, image.getNumLayers() > 1, image.getNumLayers(), (bgfx::TextureFormat::Enum)(image.getFormat()), flags, mem);
    }

    ~Texture() {
        if (bgfx::isValid(m_texture))
            bgfx::destroy(m_texture);
    }

    Texture(const Texture& o) = delete;
    Texture(Texture&& o) {
        *this = std::move(o);
    }

    Texture& operator=(const Texture& o) = delete;
    Texture& operator=(Texture&& o) {
        ti = o.ti;
        flags = o.flags;
        m_texture = o.m_texture;
        o.m_texture = {bgfx::kInvalidHandle};
        return *this;
    }

    bgfx::TextureHandle getHandle() const { return m_texture; }
    bool isValid() const { return bgfx::isValid(m_texture); }

    uint16_t getWidth() const {return ti.width;}
    uint16_t getHeight() const {return ti.height;}
    uint16_t getDepth() const {return ti.depth;}
    vec3u getDim() const {return {ti.width, ti.height, ti.depth};}

    void resize(const vec3u& newSize);
    void resize(const vec2u& newSize) {resize(vec3u{newSize, 1});}

    bool setImageData(const resource::ImageData& image, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t layer = 0, uint8_t mip = 0);
    
    /**
     * @brief Get the Image Data object. Allocates and queues a texture read operation.
     * Data will not be ready until the operation is completed by the rendering thread.
     * 
     * @param mip 
     * @return 
     */
    std::future<resource::ImageData> getImageData(uint8_t mip = 0) const;

private:
    bgfx::TextureInfo ti;
    uint64_t flags;
    bgfx::TextureHandle m_texture{bgfx::kInvalidHandle};
};

}

#endif // DIRTBOX_TEXTURE_H