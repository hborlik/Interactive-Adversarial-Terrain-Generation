# ---- Executable ----
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.inl")

CPMAddPackage(
    NAME bgfxc
    GITHUB_REPOSITORY widberg/bgfx.cmake
    GIT_TAG master
    # DOWNLOAD_ONLY True
)

CPMAddPackage(
    NAME glfw3
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.3.2
    # DOWNLOAD_ONLY True
)

if(glfw3_ADDED)
    add_library(glfw3 INTERFACE)
    target_include_directories(glfw3 INTERFACE ${glfw3_SOURCE_DIR}/include)
    target_link_libraries(glfw3 INTERFACE glfw)
endif()

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# CPMAddPackage(
#     NAME imgui
#     GITHUB_REPOSITORY ocornut/imgui
#     VERSION 1.79
# )

# if(imgui_ADDED)
# add_library(imgui INTERFACE)
# target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})
# endif()

add_subdirectory(resources)

add_executable(dirtbox ${SOURCES} ${HEADERS})
set_target_properties(dirtbox PROPERTIES 
                            CXX_STANDARD 17
                            CXX_STANDARD_REQUIRED TRUE
                            CXX_EXTENSIONS OFF
)

#target_link_libraries(dirtbox tinyobjloader stbimageloader)
target_include_directories(dirtbox PUBLIC src)
target_link_libraries(dirtbox glfw3 GL X11 dl pthread rt bgfx dear-imgui stdc++fs nlohmann_json::nlohmann_json)
add_dependencies(dirtbox dirtbox-shaders)

message("CMAKE_CXX17_STANDARD_COMPILE_OPTION = ${CMAKE_CXX17_STANDARD_COMPILE_OPTION}")

