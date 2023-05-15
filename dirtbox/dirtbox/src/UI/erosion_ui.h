/**
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-06
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_EROSION_WIDGET_H
#define DIRTBOX_EROSION_WIDGET_H

#include <memory>
#include <future>
#include <atomic>

#include <UI/ui_content.h>
#include <util/parameter.h>

namespace dirtbox {

// forward declarations
namespace terrain {
class Erosion;
}

class TerrainRenderer;

class ErosionWindow : public UIContent {
public:
    using UIContent::UIContent;
    virtual ~ErosionWindow() {}

    void OnGUIUpdate() override;

private:
    void updateParamCache();

    std::shared_ptr<terrain::Erosion> erosion;
    util::ParameterList<float> parameter_cache;
};

}

#endif // DIRTBOX_EROSION_WIDGET_H