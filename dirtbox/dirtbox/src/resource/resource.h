/**
 
 * @author Hunter Borlik
 * @brief disk loaded resources
 * @version 0.1
 * @date 2021-03-09
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_RESOURCE_H
#define DIRTBOX_RESOURCE_H

#include <memory>
#include <map>

namespace dirtbox::resource {

class ResourceBase {
public:
    virtual ~ResourceBase() = default;
};

template<typename T>
class Resource : private ResourceBase {
public:
    virtual ~Resource() = default;

private:
    friend class ResourceManager;

    template<typename... Args>
    static T* Create(const std::string& file_name, Args&&... args) {
        return new T(file_name, std::forward<Args>(args)...);
    }

};

}

#endif // DIRTBOX_RESOURCE_H