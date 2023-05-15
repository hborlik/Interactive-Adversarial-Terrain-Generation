/**
 * @brief 
 * @date 2021-05-14
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_UI_MANAGER_H
#define DIRTBOX_UI_MANAGER_H

#include <util/vec.h>
#include <util/delegate.h>

namespace dirtbox {

class UIContext {
public:
    void NotifyWindowSizeChanged(const vec2u& size) noexcept {
        window_size = size;
        WindowSizeChange(window_size);
    }

    void SetUIScale(float scale) noexcept {ui_scale = scale;}

    float GetUIScale() const {return ui_scale;}

    util::Event<void(const vec2u&)> WindowSizeChange;

private:
    vec2u window_size;
    float ui_scale;
};


}

#endif // DIRTBOX_UI_MANAGER_H