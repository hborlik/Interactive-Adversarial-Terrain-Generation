/**
 
 * @brief 
 * @version 0.1
 * @date 2021-03-24
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_BUFFER_H
#define DIRTBOX_BUFFER_H

#include <bgfx/bgfx.h>

namespace dirtbox {

template<typename T>
class Buffer {
public:

    ~Buffer() {
        bgfx::destroy(m_handle);
    }

    T getHandle() const {return m_handle;}

private:
    T m_handle{bgfx::kInvalidHandle};
};

using VectexBuffer = Buffer<bgfx::VertexBufferHandle>;
using IndexBuffer = Buffer<bgfx::IndexBufferHandle>;

}

#endif // DIRTBOX_BUFFER_H