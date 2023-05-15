#include <UI/menu_bar.h>

#include <filesystem>

#include <imgui/imgui.h>
#include <terrain/terrain.h>
#include <third_party/L2DFileDialog.h>
#include <app.h>

using namespace std::filesystem;
using namespace std::chrono_literals;

namespace dirtbox {

FileDialogWindow::FileDialogWindow(UIContext& context) : 
    UIContent{context},
    currentPath{current_path()} {

}

void FileDialogWindow::OnGUIUpdate() {
    const float scale = Context.GetUIScale();

    static int fileDialogFileSelectIndex = 0;
    static int fileDialogFolderSelectIndex = 0;
    static std::string fileDialogCurrentFile = "";
    static std::string fileDialogCurrentFolder = "";
    static char fileDialogError[500] = "";
    static FileDialogSortOrder fileNameSortOrder = FileDialogSortOrder::None;
    static FileDialogSortOrder sizeSortOrder = FileDialogSortOrder::None;
    static FileDialogSortOrder dateSortOrder = FileDialogSortOrder::None;
    static FileDialogSortOrder typeSortOrder = FileDialogSortOrder::None;


    if (enabled) {
        ImGui::SetNextWindowSize(ImVec2(740.0f * scale, 410.0f * scale));
        ImGui::Begin("Select a file", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        std::vector<std::filesystem::directory_entry> files;
        std::vector<std::filesystem::directory_entry> folders;
        try {
            for (auto& p : std::filesystem::directory_iterator(currentPath)) {
                if (p.is_directory()) {
                    folders.push_back(p);
                }
                else {
                    files.push_back(p);
                }
            }
        }
        catch (...) {}

        ImGui::Text("%s", currentPath.c_str());

        ImGui::BeginChild("Directories##1", ImVec2(200 * scale, 300 * scale), true, ImGuiWindowFlags_HorizontalScrollbar);

        if (ImGui::Selectable("..", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                currentPath = std::filesystem::path(currentPath).parent_path().string();
            }
        }
        for (int i = 0; i < folders.size(); ++i) {
            if (ImGui::Selectable(folders[i].path().stem().string().c_str(), i == fileDialogFolderSelectIndex, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
                fileDialogCurrentFile = "";
                if (ImGui::IsMouseDoubleClicked(0)) {
                    currentPath = folders[i].path().string();
                    fileDialogFolderSelectIndex = 0;
                    fileDialogFileSelectIndex = 0;
                    ImGui::SetScrollHereY(0.0f);
                    fileDialogCurrentFolder = "";
                }
                else {
                    fileDialogFolderSelectIndex = i;
                    fileDialogCurrentFolder = folders[i].path().stem().string();
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Files##1", ImVec2(516 * scale, 300 * scale), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Columns(4);
        static float initialSpacingColumn0 = 230.0f * scale;
        if (initialSpacingColumn0 > 0) {
            ImGui::SetColumnWidth(0, initialSpacingColumn0);
            initialSpacingColumn0 = 0.0f;
        }
        static float initialSpacingColumn1 = 80.0f * scale;
        if (initialSpacingColumn1 > 0) {
            ImGui::SetColumnWidth(1, initialSpacingColumn1);
            initialSpacingColumn1 = 0.0f;
        }
        static float initialSpacingColumn2 = 80.0f * scale;
        if (initialSpacingColumn2 > 0) {
            ImGui::SetColumnWidth(2, initialSpacingColumn2);
            initialSpacingColumn2 = 0.0f;
        }
        if (ImGui::Selectable("File")) {
            sizeSortOrder = FileDialogSortOrder::None;
            dateSortOrder = FileDialogSortOrder::None;
            typeSortOrder = FileDialogSortOrder::None;
            fileNameSortOrder = (fileNameSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
        }
        ImGui::NextColumn();
        if (ImGui::Selectable("Size")) {
            fileNameSortOrder = FileDialogSortOrder::None;
            dateSortOrder = FileDialogSortOrder::None;
            typeSortOrder = FileDialogSortOrder::None;
            sizeSortOrder = (sizeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
        }
        ImGui::NextColumn();
        if (ImGui::Selectable("Type")) {
            fileNameSortOrder = FileDialogSortOrder::None;
            dateSortOrder = FileDialogSortOrder::None;
            sizeSortOrder = FileDialogSortOrder::None;
            typeSortOrder = (typeSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
        }
        ImGui::NextColumn();
        if (ImGui::Selectable("Date")) {
            fileNameSortOrder = FileDialogSortOrder::None;
            sizeSortOrder = FileDialogSortOrder::None;
            typeSortOrder = FileDialogSortOrder::None;
            dateSortOrder = (dateSortOrder == FileDialogSortOrder::Down ? FileDialogSortOrder::Up : FileDialogSortOrder::Down);
        }
        ImGui::NextColumn();
        ImGui::Separator();

        // Sort files
        if (fileNameSortOrder != FileDialogSortOrder::None) {
            std::sort(files.begin(), files.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                if (fileNameSortOrder == FileDialogSortOrder::Down) {
                    return a.path().filename().string() > b.path().filename().string();
                }
                else {
                    return a.path().filename().string() < b.path().filename().string();
                }
                });
        }
        else if (sizeSortOrder != FileDialogSortOrder::None) {
            std::sort(files.begin(), files.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                if (sizeSortOrder == FileDialogSortOrder::Down) {
                    return a.file_size() > b.file_size();
                }
                else {
                    return a.file_size() < b.file_size();
                }
                });
        }
        else if (typeSortOrder != FileDialogSortOrder::None) {
            std::sort(files.begin(), files.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                if (typeSortOrder == FileDialogSortOrder::Down) {
                    return a.path().extension().string() > b.path().extension().string();
                }
                else {
                    return a.path().extension().string() < b.path().extension().string();
                }
                });
        }
        else if (dateSortOrder != FileDialogSortOrder::None) {
            std::sort(files.begin(), files.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                if (dateSortOrder == FileDialogSortOrder::Down) {
                    return a.last_write_time() > b.last_write_time();
                }
                else {
                    return a.last_write_time() < b.last_write_time();
                }
                });
        }

        for (int i = 0; i < files.size(); ++i) {
            if (ImGui::Selectable(files[i].path().filename().string().c_str(), i == fileDialogFileSelectIndex, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
                fileDialogFileSelectIndex = i;
                fileDialogCurrentFile = files[i].path().filename().string();
                fileDialogCurrentFolder = "";
            }
            ImGui::NextColumn();
            ImGui::TextUnformatted(std::to_string(files[i].file_size()).c_str());
            ImGui::NextColumn();
            ImGui::TextUnformatted(files[i].path().extension().string().c_str());
            ImGui::NextColumn();
            auto ftime = files[i].last_write_time();
            auto st = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now());
            std::time_t tt = std::chrono::system_clock::to_time_t(st);
            std::tm* mt = std::localtime(&tt);
            std::stringstream ss;
            ss << std::put_time(mt, "%F %R");
            ImGui::TextUnformatted(ss.str().c_str());
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        std::string selectedFilePath = path{fileDialogCurrentFolder.size() > 0 ? fileDialogCurrentFolder : fileDialogCurrentFile};
        selectedFilePath.resize(500);
        ImGui::PushItemWidth(ImGui::GetWindowWidth());
        if (ImGui::InputText("", selectedFilePath.data(), selectedFilePath.size())) {
            fileDialogCurrentFile = selectedFilePath;
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

        if (ImGui::Button("New folder")) {
            ImGui::OpenPopup("NewFolderPopup");
        }
        ImGui::SameLine();

        static bool disableDeleteButton = false;
        disableDeleteButton = (fileDialogCurrentFolder == "");
        if (disableDeleteButton) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("Delete folder")) {
            ImGui::OpenPopup("DeleteFolderPopup");
        }
        if (disableDeleteButton) {
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
        }

        ImVec2 center(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopup("NewFolderPopup", ImGuiWindowFlags_Modal)) {
            ImGui::Text("Enter a name for the new folder");
            static char newFolderName[500] = "";
            static char newFolderError[500] = "";
            ImGui::InputText("", newFolderName, sizeof(newFolderName));
            if (ImGui::Button("Create##1")) {
                if (strlen(newFolderName) <= 0) {
                    strcpy(newFolderError, "Folder name can't be empty");
                }
                else {
                    std::string newFilePath = currentPath + (currentPath.back() == '\\' ? "" : "\\") + newFolderName;
                    std::filesystem::create_directory(newFilePath);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##1")) {
                strcpy(newFolderName, "");
                strcpy(newFolderError, "");
                ImGui::CloseCurrentPopup();
            }
            ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), newFolderError);
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopup("DeleteFolderPopup", ImGuiWindowFlags_Modal)) {
            ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), "Are you sure you want to delete this folder?");
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
            ImGui::TextUnformatted(fileDialogCurrentFolder.c_str());
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
            if (ImGui::Button("Yes")) {
                std::filesystem::remove(currentPath + (currentPath.back() == '\\' ? "" : "\\") + fileDialogCurrentFolder);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120 * scale);

        if (ImGui::Button("Cancel")) {
            fileDialogFileSelectIndex = 0;
            fileDialogFolderSelectIndex = 0;
            fileDialogCurrentFile = "";
            enabled = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Choose")) {
            if (type == FileDialogType::SelectFolder) {
                if (fileDialogCurrentFolder == "") {
                    strcpy(fileDialogError, "Error: You must select a folder!");
                }
                else {
                    SelectedPath = path{currentPath} / path{fileDialogCurrentFolder};
                    fileDialogFileSelectIndex = 0;
                    fileDialogFolderSelectIndex = 0;
                    fileDialogCurrentFile = "";
                    enabled = false;
                    did_select = true;
                }
            } else if (type == FileDialogType::OpenFile) {
                if (fileDialogCurrentFile == "") {
                    strcpy(fileDialogError, "Error: You must select a file!");
                }
                else {
                    SelectedPath = path{currentPath} / path{fileDialogCurrentFile};
                    fileDialogFileSelectIndex = 0;
                    fileDialogFolderSelectIndex = 0;
                    fileDialogCurrentFile = "";
                    enabled = false;
                    did_select = true;
                }
            }
        }

        if (strlen(fileDialogError) > 0) {
            ImGui::TextColored(ImColor(1.0f, 0.0f, 0.2f, 1.0f), fileDialogError);
        }

        ImGui::End();
    }
}

MenuBar::MenuBar(UIContext& context) : 
    UIContent{context},
    gen_editor{std::make_shared<GANGeneratorEditor>(Context)},
    erosion_editor{std::make_shared<ErosionWindow>(Context)},
    dialogWindow{std::make_shared<FileDialogWindow>(Context)} {
        erosion_editor->enabled = false;
        gen_editor->enabled = false;
    }

void MenuBar::OnGUIUpdate() {
    
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowFileMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("GAN", NULL, &gen_editor->enabled);
            ImGui::MenuItem("Erosion", NULL, &erosion_editor->enabled);
            ImGui::MenuItem("ImGui Demo Window", NULL, &demo_window_open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();        
    }

    if (fdo != FileDialogOperation::None) {
        if (dialogWindow->didSelect()) {
            std::string filename = dialogWindow->SelectedPath;
            switch (fdo) {
                case FileDialogOperation::TerrainFile:
                    GetTerrainManager().getTerrain()->loadTerrain(filename);
                    break;
                case FileDialogOperation::SaveTerrainPng:
                    GetTerrainManager().getTerrain()->saveTerrain(filename);
                    break;
            }
            fdo = FileDialogOperation::None;
        }
    }

    if (settings_menu_open)
        ShowSettingsMenu();

    if (demo_window_open)
        ImGui::ShowDemoWindow();

    if (gen_editor->enabled)
        gen_editor->OnGUIUpdate();

    if (erosion_editor->enabled)
        erosion_editor->OnGUIUpdate();

    if (dialogWindow->enabled)
        dialogWindow->OnGUIUpdate();
}

void MenuBar::ShowFileMenu() {
    ImGui::MenuItem("(file)", NULL, false, false);
    // if (ImGui::MenuItem("New")) {}
    // if (ImGui::MenuItem("Open", "Ctrl+O")) {}
    if (ImGui::MenuItem("Import Terrain")) {
        dialogWindow->ShowFileDialog(FileDialogWindow::FileDialogType::OpenFile);
        fdo = FileDialogOperation::TerrainFile;
    }
    // if (ImGui::BeginMenu("Open Recent"))
    // {
    //     ImGui::MenuItem("fish_hat.c");
    //     ImGui::MenuItem("fish_hat.inl");
    //     ImGui::MenuItem("fish_hat.h");
    //     if (ImGui::BeginMenu("More.."))
    //     {
    //         ImGui::MenuItem("Hello");
    //         ImGui::MenuItem("Sailor");
    //         if (ImGui::BeginMenu("Recurse.."))
    //         {
    //             ShowFileMenu();
    //             ImGui::EndMenu();
    //         }
    //         ImGui::EndMenu();
    //     }
    //     ImGui::EndMenu();
    // }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}

    if (ImGui::MenuItem("Save As..")) {
        dialogWindow->ShowFileDialog(FileDialogWindow::FileDialogType::OpenFile);
        fdo = FileDialogOperation::SaveTerrainPng;
    }

    ImGui::Separator();
    // if (ImGui::BeginMenu("Options"))
    // {
        // static bool enabled = true;
        // ImGui::MenuItem("Enabled", "", &enabled);
        // ImGui::BeginChild("child", ImVec2(0, 60), true);
        // for (int i = 0; i < 10; i++)
        //     ImGui::Text("Scrolling Text %d", i);
        // ImGui::EndChild();
        // static float f = 0.5f;
        // static int n = 0;
        // ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        // ImGui::InputFloat("Input", &f, 0.1f);
        // ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
    //     ImGui::EndMenu();
    // }

    // if (ImGui::BeginMenu("Colors"))
    // {
    //     float sz = ImGui::GetTextLineHeight();
    //     for (int i = 0; i < ImGuiCol_COUNT; i++)
    //     {
    //         const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
    //         ImVec2 p = ImGui::GetCursorScreenPos();
    //         ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+sz, p.y+sz), ImGui::GetColorU32((ImGuiCol)i));
    //         ImGui::Dummy(ImVec2(sz, sz));
    //         ImGui::SameLine();
    //         ImGui::MenuItem(name);
    //     }
    //     ImGui::EndMenu();
    // }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    // if (ImGui::BeginMenu("Options")) // <-- Append!
    // {
    //     static bool b = true;
        // ImGui::Checkbox("SomeOption", &b);
        if (ImGui::MenuItem("Open Settings")) {
            settings_menu_open = true;
        }
    //     ImGui::EndMenu();
    // }

    // if (ImGui::BeginMenu("Disabled", false)) // Disabled
    // {
    //     IM_ASSERT(0);
    // }
    // if (ImGui::MenuItem("Checked", NULL, true)) {}
    if (ImGui::MenuItem("Quit", "")) {}
}

void MenuBar::ShowSettingsMenu() {
    if (ImGui::Begin("Settings", &settings_menu_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        float scale = Context.GetUIScale();
        if (ImGui::SliderFloat("UI Scale", &scale, 0.5f, 2.0f)) {
            Context.SetUIScale(scale);
        }

        if (ImGui::Checkbox("Debug wireframe", &m_wireframe) )
        {
            bgfx::setDebug(m_wireframe
                ? BGFX_DEBUG_WIREFRAME
                : BGFX_DEBUG_NONE
                );
        }

        ImGui::SameLine();

        if (ImGui::Checkbox("Show Stats", &m_showbgfxStats) )
        {
            bgfx::setDebug(m_showbgfxStats
                ? BGFX_DEBUG_STATS
                : BGFX_DEBUG_NONE
                );
        }

        if (ImGui::CollapsingHeader("Terrain Rendering")) {

            auto& renderer = GetTerrainRenderer();

            bool m_cull = renderer.getUniforms().cull == 1.0f;
            if (ImGui::Checkbox("Cull", &m_cull) )
            {
                renderer.getUniforms().cull = m_cull ? 1.0f : 0.0f;
            }

            ImGui::SameLine();

            bool m_freeze = renderer.getUniforms().freeze == 1.0f;
            if (ImGui::Checkbox("Freeze subdividing", &m_freeze) )
            {
                renderer.getUniforms().freeze = m_freeze ? 1.0f : 0.0f;
            }


            float ppmf = renderer.getPixelLengthTarget();
            if (ImGui::SliderFloat("Pixels per edge", &ppmf, 1, 20)) {
                renderer.setPixelLengthTarget(ppmf);
            }

            int gpuSlider = (int)renderer.getUniforms().gpuSubd;

            if (ImGui::SliderInt("Triangle Patch level", &gpuSlider, 0, 3) )
            {
                renderer.forceRestart();
                renderer.getUniforms().gpuSubd = float(gpuSlider);
            }
            ImGui::Text("Some variables require rebuilding the subdivide buffers and causes a stutter.");

            static const char* s_shaderOptions[] =
            {
                "Normal",
                "Diffuse"
            };

            int shadingMode = (int)renderer.getShadingMode();
            if(ImGui::Combo("Shading", &shadingMode, s_shaderOptions, 2)) {
                renderer.setShadingMode((TerrainRenderer::ShadingMode)shadingMode);
            }
            ImGui::SliderFloat("Height Factor", &renderer.getUniforms().dmapFactor, 0, 3);
        }
    }




    // const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "PPPP", "QQQQQQQQQQ", "RRR", "SSSS" };
    // static const char* current_item = NULL;

    // if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
    // {
    //     for (int n = 0; n < IM_ARRAYSIZE(items); n++)
    //     {
    //         ImGui::PushID(n);
    //         bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
    //         if (ImGui::Selectable(items[n], is_selected))
    //             current_item = items[n];
    //         if (is_selected)
    //             ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
    //         ImGui::PopID();
    //     }
    //     ImGui::EndCombo();
    // }

    ImGui::End();
}


        
}