/**
 
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-03-02
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_UI_WINDOW_H
#define DIRTBOX_UI_WINDOW_H

#include <UI/dbui.h>

namespace dirtbox {

class UIContent {
public:
    explicit UIContent(UIContext& context) : Context{context} {}
    virtual ~UIContent() {}

    virtual void OnGUIUpdate() = 0;

    void Enable() {enabled = true;}
    void Disable() {enabled = false;}
    
    bool enabled = true;

protected:
    UIContext& Context;
};


}

#endif // DIRTBOX_UI_WINDOW_H