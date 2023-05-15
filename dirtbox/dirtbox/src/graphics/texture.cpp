#include <graphics/texture.h>

#include <thread>
#include <memory>

#include <core/core.h>

namespace dirtbox::graphics {

void Texture::resize(const vec3u& newSize) {
    if (vec3u{ti.width, ti.height, ti.depth} == newSize)
        return;
    bgfx::calcTextureSize(ti, newSize.x(), newSize.y(), newSize.z(), false, false, 1, ti.format);
    
    if (bgfx::isValid(m_texture))
        bgfx::destroy(m_texture);
    
    if (newSize.z() > 1) {
        m_texture = bgfx::createTexture3D(ti.width, ti.height, ti.depth, false, ti.format, flags, nullptr);
    } else {
        m_texture = bgfx::createTexture2D(ti.width, ti.height, false, 1, ti.format, flags, nullptr);
    }
}

bool Texture::setImageData(const resource::ImageData& image, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t layer, uint8_t mip) {
    if (x + width > ti.width || y + height > ti.height || width > image.getWidth() || height > image.getHeight())
        return false;
    auto* mem = bgfx::copy(image.get()->m_data, image.getSize());
    bgfx::updateTexture2D(m_texture, layer, mip, x, y, width, height, mem);
    return true;
}

std::future<resource::ImageData> Texture::getImageData(uint8_t mip) const {
    auto nimg = resource::ImageData::CreateImage({ti.width, ti.height}, ti.format);
    uint32_t fid = bgfx::readTexture(m_texture, nimg.get()->m_data, mip);
    // start wait thread that returns image once desired frame is reached
    auto fut = std::async([fid](resource::ImageData img) {
        Core::Get().FrameEvent.wait();
        return std::move(img);
    }, std::move(nimg));
    return fut;
}

}