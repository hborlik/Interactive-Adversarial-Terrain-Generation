#include <UI/gan_generator_editor.h>

#include <imgui/imgui.h>
#include <app.h>
#include <third_party/L2DFileDialog.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace dirtbox {

const vec3f BrushStyleToColor[3] = {
    {1, 0, 0}, // Hill
    {0, 1, 0}, // Valley
    {0, 0, 1} // Elevation
};

const uint8_t BrushStyleToBrushSize[3] = {
    1, // Hill
    1, // Valley
    15 // Elevation
};

GANGeneratorEditor::GANGeneratorEditor(UIContext& context) : 
    UIContent{context},
    gan_input{512, 512, bgfx::TextureFormat::RGBA8, 1, BGFX_TEXTURE_READ_BACK} {
        enabled = false;
        ClearGanInput();
}

void GANGeneratorEditor::OnGUIUpdate() {

    if (file_dialog_open) {
        std::string filename;
        FileDialog::ShowFileDialog(&file_dialog_open, Context.GetUIScale(), filename);
        if (filename.length() > 0) {
            gan_generator.GetInputImage().loadImage(filename);
        }
    }

    if (m_gpu_tex_dirty)
        ForceGPUTextureUpdate();

    auto io = ImGui::GetIO();

    if (ImGui::Begin("GAN", &enabled, ImGuiWindowFlags_AlwaysAutoResize)) {

        if (ImGui::Button("Load Input Image")) {
            file_dialog_open = true;
        }

        float ui_scale = Context.GetUIScale();
        ImGui::ImageButton(gan_input.getHandle(), ImGui::IMGUI_FLAGS_NONE, 0, {512 * ui_scale, 512 * ui_scale});
        vec2f mousePos{(io.MousePos.x - ImGui::GetItemRectMin().x) / ui_scale, (io.MousePos.y - ImGui::GetItemRectMin().y) / ui_scale};
        vec2f prevMousePos{mousePos.x() - io.MouseDelta.x, mousePos.y() - io.MouseDelta.y};
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ApplyBrushGanInput(
                prevMousePos,
                mousePos,
                BrushStyleToColor[(uint16_t)brushStyle] * (brushStyle == BrushStyle::ElevationTarget ? elevationStrength : 1.0f), 
                BrushStyleToBrushSize[(uint16_t)brushStyle]);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ApplyBrushGanInput(
                mousePos,
                mousePos,
                BrushStyleToColor[(uint16_t)brushStyle] * (brushStyle == BrushStyle::ElevationTarget ? elevationStrength : 1.0f), 
                BrushStyleToBrushSize[(uint16_t)brushStyle]);
        }

        ImGui::Separator();

        if (ImGui::Selectable("Hill", brushStyle == BrushStyle::Hill)) {
            brushStyle = BrushStyle::Hill;
        }
        if (ImGui::Selectable("Valley", brushStyle == BrushStyle::Valley)) {
            brushStyle = BrushStyle::Valley;
        }
        if (ImGui::Selectable("Elevation target", brushStyle == BrushStyle::ElevationTarget)) {
            brushStyle = BrushStyle::ElevationTarget;
        }
        ImGui::SliderFloat("Elevation Strength", &elevationStrength, 0, 1);

        ImGui::Separator();

        if (ImGui::Button("Clear")) {
            error_message = "";
            ClearGanInput();
        }

        if (ImGui::Button("Generate")) {
            try {
                gan_generator.Generate(*GetTerrainManager().getTerrain());
                error_message = "";
            } catch (const std::runtime_error& e) {
                error_message = e.what();
            }
        }

        if (error_message.size() > 0)
            ImGui::TextColored(ImVec4{1, 0, 0, 1}, "Exception: %s", error_message.c_str());
    }

    ImGui::End();
}

void GANGeneratorEditor::ApplyBrushGanInput(const vec2f& pos_from, const vec2f& pos_to, const vec3f& value, uint8_t brushSize) {
    if (pos_from.x() > 0 && pos_from.x() < gan_input.getWidth() && pos_from.y() > 0 && pos_from.y() < gan_input.getHeight() &&
        pos_to.x() > 0 && pos_to.x() < gan_input.getWidth() && pos_to.y() > 0 && pos_to.y() < gan_input.getHeight()) {
        uint8_t* data = (uint8_t*)gan_generator.GetInputImage().get()->m_data;
        vec2f dir = ((vec2f)(pos_to - pos_from)).normalized();
        const float dist = ((vec2f)(pos_to - pos_from)).leng();
        for (vec2f pos{}; pos.leng() <= dist; pos += dir)
            for (int x = (pos + pos_from).x(); x < std::min<int>(gan_input.getHeight(), (pos + pos_from).x() + brushSize); ++x)
                for (int y = (pos + pos_from).y(); y < std::min<int>(gan_input.getWidth(), (pos + pos_from).y() + brushSize); ++y) {
                    data[0 + 4 * (x + y * 512)] = std::min<uint16_t>(255, value.x() > 0 ? value.x() * 255 : data[0 + 4 * (x + y * 512)]);
                    data[1 + 4 * (x + y * 512)] = std::min<uint16_t>(255, value.y() > 0 ? value.y() * 255 : data[1 + 4 * (x + y * 512)]);
                    data[2 + 4 * (x + y * 512)] = std::min<uint16_t>(255, value.z() > 0 ? value.z() * 255 : data[2 + 4 * (x + y * 512)]);
                    data[3 + 4 * (x + y * 512)] = 127;
                }
        m_gpu_tex_dirty = true;
    }
}

void GANGeneratorEditor::ClearGanInput() {
    uint8_t* data = (uint8_t*)gan_generator.GetInputImage().get()->m_data;
    for (int x = 0; x < gan_input.getHeight(); ++x)
        for (int y = 0; y < gan_input.getWidth(); ++y) {
            data[0 + 4 * (x + y * 512)] = 0;
            data[1 + 4 * (x + y * 512)] = 0;
            data[2 + 4 * (x + y * 512)] = 0;
            data[3 + 4 * (x + y * 512)] = 127;
        }
    m_gpu_tex_dirty = true;
}

void GANGeneratorEditor::ForceGPUTextureUpdate() {
    gan_input.setImageData(gan_generator.GetInputImage(), 0, 0, 512, 512);
        // bgfx::updateTexture2D(gan_input.getHandle(), 0, 0, 0, 0, 512, 512, 
        //     bgfx::makeRef(gan_generator.GetInputImage().get()->m_data, gan_generator.GetInputImage().getSize()));
}
    
}