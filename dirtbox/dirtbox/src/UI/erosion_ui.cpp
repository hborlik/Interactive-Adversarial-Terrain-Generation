#include <UI/erosion_ui.h>
#include <imgui/imgui.h>
#include <graphics/renderers/terrain_renderer.h>
#include <terrain/erosion.h>
#include <terrain/etes_erosion.h>
#include <terrain/erosion_model2.h>

#include <app.h>

namespace dirtbox {


void ErosionWindow::OnGUIUpdate() {
    ImGui::SetNextWindowPos(
        ImVec2(),
        ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(
        ImVec2(400,400),
        ImGuiCond_FirstUseEver
    );
    ImGui::Begin("Terrain Erosion Settings", &enabled);

    const std::vector<std::string>& items = {"Erosion2SimulationGPU"};
    static int current_item = 0;
    bool selected_erosion_changed = false;

    if (ImGui::BeginCombo("##combo", items[current_item].c_str())) // The second parameter is the label previewed before opening the combo.
    {
        for (int i = 0; i < items.size(); ++i)
        {
            const auto& item = items[i];
            ImGui::PushID(item.c_str());
            bool is_selected = (current_item == i); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(item.c_str(), is_selected)) {
                current_item = i;
                selected_erosion_changed = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    if (selected_erosion_changed) {
        erosion = GetTerrainManager().createErosion(items[current_item]);
        updateParamCache();
    }

    if (erosion) {
        if (!erosion->isRunning()) {
            if (ImGui::Button("Start")) {
                erosion->startErosionTask();
            }

            ImGui::Separator();

            for (auto& p : parameter_cache) {
                if (p.min < p.max) {
                    if(ImGui::SliderFloat(p.name.c_str(), &p.value, p.min, p.max))
                        erosion->setParam(p.name, p.value);
                } else {
                    if(ImGui::InputFloat(p.name.c_str(), &p.value))
                        erosion->setParam(p.name, p.value);
                }
            }

        } else {
            if (ImGui::Button("stop")) {
                erosion->stopErosionTask();
            }
            erosion->update();
            ImGui::ProgressBar(erosion->getProgress());
        }
    }

    ImGui::End();
}

void ErosionWindow::updateParamCache() {
    parameter_cache = erosion->getParams();
}

}