#include <resource/resource.h>
#include <resource/resource_manager.h>

#include <filesystem>

#include <bimg/bimg.h>
#include <bx/file.h>
#include <util/box_utils.h>

namespace dirtbox::resource {

void ResourceManager::SaveImage(const ImageData& image, const std::string& file_name) {
    std::vector<uint8_t> buf;
    image.writeImagePNG(buf);
    //TODO
}

}