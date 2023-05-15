#include <resource/image.h>

#include <util/box_utils.h>
#include <bx/bx.h>
#include <bx/readerwriter.h>
#include <bx/file.h>

namespace dirtbox::resource {

ImageData::ImageData(bimg::ImageContainer* image) : 
    image{image} {
    if (!image)
        throw std::runtime_error{"Invalid image data"};
}

ImageData::ImageData(const std::string& file_name, bgfx::TextureFormat::Enum format) :
    image{imageLoad(file_name.c_str(), format)} {
    if (!image)
        throw std::runtime_error{"Invalid image data"};
}

ImageData::~ImageData() {
    if (image)
        bimg::imageFree(image);
}

ImageData ImageData::getAsFormat(bgfx::TextureFormat::Enum format) const {
    auto* newImage = bimg::imageConvert(image->m_allocator, bimg::TextureFormat::Enum(format), *image, true);
    return ImageData{newImage};
}

bool ImageData::writeImagePNG(std::vector<uint8_t>& buf) const {
    bx::MemoryBlock imgdata{getAllocator()};
    bx::MemoryWriter writer{&imgdata};
    bx::Error result;
    {
        auto formatted_img = getAsFormat(bgfx::TextureFormat::RGBA8);
        bimg::imageWritePng(&writer, getWidth(), getHeight(), 4 * getWidth(), formatted_img.get()->m_data, bimg::TextureFormat::RGBA8, false, &result);
    }
    buf.reserve(imgdata.getSize());
    buf.clear();
    uint8_t* data = (uint8_t*)imgdata.more();
    for (uint32_t i = 0; i < imgdata.getSize(); ++i) {
        buf.emplace_back(data[i]);
    }

    return result.isOk();
}

bool ImageData::loadImage(const std::string& file_name) {

    auto nimage = imageLoad(file_name.c_str(), (bgfx::TextureFormat::Enum)image->m_format);

    if (nimage) {
        if (image)
            bimg::imageFree(image);
        image = nimage;
        return true;
    }
    return false;
}

bool ImageData::saveImage(const std::string& file_name) const {
    return savePng(file_name.c_str(), getWidth(), getHeight(), image->m_data, false);
}

ImageData ImageData::CreateSolidImage(const vec2u& size, bgfx::TextureFormat::Enum format, uint32_t value) {
    auto* newImage  = bimg::imageAlloc(getAllocator(),
                    (bimg::TextureFormat::Enum)format, 
                    uint16_t(size.x()), 
                    uint16_t(size.y()),
                    uint16_t(1), 
                    1, 
                    false, 
                    false
                    );
    uint32_t* dst = (uint32_t*)newImage->m_data;
    for (uint32_t ii = 0; ii < size.x()*size.y(); ++ii)
    {
        *dst++ = value;
    }
    return ImageData{newImage};
}

ImageData ImageData::CreateImage(const vec2u& size, bgfx::TextureFormat::Enum format) {
    auto* newImage  = bimg::imageAlloc(getAllocator(),
                    (bimg::TextureFormat::Enum)format, 
                    uint16_t(size.x()), 
                    uint16_t(size.y()),
                    uint16_t(1), 
                    1, 
                    false, 
                    false
                    );
    return ImageData{newImage};
}

}