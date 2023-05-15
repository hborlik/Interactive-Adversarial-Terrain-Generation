/*
    Copyright 2020 Limeoats

       Licensed under the Apache License, Version 2.0 (the "License");
       you may not use this file except in compliance with the License.
       You may obtain a copy of the License at
    
           http://www.apache.org/licenses/LICENSE-2.0
    
       Unless required by applicable law or agreed to in writing, software
       distributed under the License is distributed on an "AS IS" BASIS,
       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
       See the License for the specific language governing permissions and
       limitations under the License.
*/

#pragma once


#include <dear-imgui/imgui.h>
#include <dear-imgui/imgui_internal.h>

#include <string>

namespace FileDialog {

    enum class FileDialogType {
        OpenFile,
        SelectFolder
    };
    enum class FileDialogSortOrder {
        Up,
        Down,
        None
    };

    void ShowFileDialog(bool* open, float scale, std::string& buffer, FileDialogType type = FileDialogType::OpenFile);

}

