/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-08
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_RENDER_CONTEXT_H
#define DIRTBOX_RENDER_CONTEXT_H

#include <util/mat.h>

namespace dirtbox {

struct RenderContext {
    mat4<float> viewMat;
    mat4<float> projMat;
};

}

#endif // DIRTBOX_RENDER_CONTEXT_H