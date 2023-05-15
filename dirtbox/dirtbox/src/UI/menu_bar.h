/**
 * @brief 
 * @version 0.1
 * @date 2021-04-27
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_MENU_BAR_H
#define DIRTBOX_MENU_BAR_H

#include <memory>
#include <UI/ui_content.h>
#include <UI/gan_generator_editor.h>
#include <UI/erosion_ui.h>

namespace dirtbox::terrain {
    class Terrain;
}

namespace dirtbox {

class FileDialogWindow : public UIContent {
public:

    enum class FileDialogType {
        OpenFile,
        SelectFolder
    };
    enum class FileDialogSortOrder {
        Up,
        Down,
        None
    };

    explicit FileDialogWindow(UIContext& context);

    void OnGUIUpdate() override;

    void ShowFileDialog(FileDialogType type) {
        enabled = true;
        did_select = false;
        this->type = type;
    }

    bool didSelect() const {return did_select;}

    std::string SelectedPath;

private:
    std::string currentPath;
    FileDialogType type;
    bool did_select = false;
};

class MenuBar : public UIContent {
public:
    explicit MenuBar(UIContext& context);
    void OnGUIUpdate() override;

    void ShowFileMenu();
    void ShowSettingsMenu();
private:
    std::shared_ptr<GANGeneratorEditor> gen_editor;
    std::shared_ptr<ErosionWindow> erosion_editor;
    std::shared_ptr<FileDialogWindow> dialogWindow;
    bool settings_menu_open = false;
    bool demo_window_open = false;

    bool m_wireframe = false;
    bool m_showbgfxStats = false;

    enum class FileDialogOperation {
        None = 0,
        TerrainFile,
        SaveTerrainPng
    };
    FileDialogOperation fdo;
};

}

#endif // DIRTBOX_MENU_BAR_H
