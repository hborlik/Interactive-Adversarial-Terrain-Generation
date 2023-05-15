/**
 * @brief 
 * @version 0.1
 * @date 2021-05-12
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_RESOURCE_MANAGER_H
#define DIRTBOX_RESOURCE_MANAGER_H

#include <resource/resource.h>
#include <resource/image.h>

namespace dirtbox::resource {

class ResourceManager {
public:

    template<typename T, typename... Args>
    static std::shared_ptr<T> Load(const std::string& file_name, Args&&... args) {
        auto obj = std::shared_ptr<T>{Resource<T>::Create(file_name, std::forward<Args>(args)...)};
        return obj;
    }

    void SaveImage(const ImageData& image, const std::string& file_name);

    // static std::shared_ptr<> LoadShader(const std::string& name) {
    // }

private:
    //std::map<std::string, std::shared_ptr<ResourceBase>> resources;
};

}

#endif // DIRTBOX_RESOURCE_MANAGER_H