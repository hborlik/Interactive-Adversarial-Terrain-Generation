/**
 * @brief 
 * @version 0.1
 * @date 2021-05-03
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_GAN_GENERATOR_EDITOR_H
#define DIRTBOX_GAN_GENERATOR_EDITOR_H

#include <UI/erosion_ui.h>
#include <graphics/texture.h>
#include <resource/image.h>
#include <terrain/generator.h>

namespace dirtbox {

class GANGeneratorEditor : public UIContent {
public:
    enum class BrushStyle {
        Hill = 0,
        Valley,
        ElevationTarget
    };

    explicit GANGeneratorEditor(UIContext& context);

    void OnGUIUpdate() override;

    void ApplyBrushGanInput(const vec2f& pos_from, const vec2f& pos_to, const vec3f& value, uint8_t brushSize);
    void ClearGanInput();
    void ForceGPUTextureUpdate();

    void SetBrushStyle(BrushStyle style) {brushStyle = style;}
private:
    graphics::Texture gan_input;
    BrushStyle brushStyle;
    terrain::HttpGanGenerator gan_generator;
    std::string error_message;
    float elevationStrength = 1.0f;
    bool m_gpu_tex_dirty = true;
    bool file_dialog_open = false;
};

}

#endif // DIRTBOX_GAN_GENERATOR_EDITOR_H