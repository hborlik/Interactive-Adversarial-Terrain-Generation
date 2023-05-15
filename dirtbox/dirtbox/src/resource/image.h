/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-09
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_IMAGE_H
#define DIRTBOX_IMAGE_H

#include <string>
#include <memory>
#include <vector>

#include <resource/resource.h>
#include <util/vec.h>
#include <bimg/bimg.h>
#include <bgfx/bgfx.h>

namespace dirtbox::resource {


class ImageData : Resource<ImageData> {
public:
    // must be owner of image container
    explicit ImageData(bimg::ImageContainer* image);
    
    ImageData(const std::string& file_name, bgfx::TextureFormat::Enum format);
    virtual ~ImageData();

    ImageData(const ImageData& o) = delete;
    ImageData(ImageData&& o) noexcept {
        (*this) = std::move(o); 
    }

    ImageData& operator=(const ImageData& o) = delete;
    ImageData& operator=(ImageData&& o) noexcept {
        image = o.image;
        o.image = nullptr;
        return *this;
    }

    bimg::ImageContainer* get() {return image;}
    const bimg::ImageContainer* get() const {return image;}

    bimg::TextureFormat::Enum   getFormat() const {return image->m_format;}
    bimg::Orientation::Enum     getOrientation() const {return image->m_orientation;}

    uint32_t    getSize() const {return image->m_size;}
    uint32_t    getOffset() const {return image->m_offset;}
    uint32_t    getWidth() const {return image->m_width;}
    uint32_t    getHeight() const {return image->m_height;}
    uint32_t    getDepth() const {return image->m_depth;}
    uint16_t    getNumLayers() const {return image->m_numLayers;}
    uint8_t     getNumMips() const {return image->m_numMips;}
    uint8_t     getBytesPerPixel() const {return bimg::getBitsPerPixel(image->m_format) / 8;}

    ImageData   getAsFormat(bgfx::TextureFormat::Enum format) const;

    bool writeImagePNG(std::vector<uint8_t>& buf) const;
    bool saveImage(const std::string& file_name) const;
    bool loadImage(const std::string& file_name);

    static ImageData CreateSolidImage(const vec2u& size, bgfx::TextureFormat::Enum format, uint32_t value);
    static ImageData CreateImage(const vec2u& size, bgfx::TextureFormat::Enum format);

private:
    bimg::ImageContainer* image;
};

using ImageDataRef = std::shared_ptr<ImageData>;


}

#endif // DIRTBOX_IMAGE_H