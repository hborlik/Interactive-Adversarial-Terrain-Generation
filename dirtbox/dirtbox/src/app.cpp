
#include <memory>
#include <iostream>

#include <app.h>
#include <graphics/camera.h>
#include <core/core.h>
#include <UI/dbui.h>
#include <UI/ui_content.h>
#include <UI/erosion_ui.h>
#include <UI/menu_bar.h>
#include <terrain/terrain.h>
#include <terrain/etes_erosion.h>
#include <terrain/erosion_model2.h>
#include <resource/resource.h>
#include <resource/image.h>
#include <imgui/imgui.h>
#include <window_events.h>
#include <window.h>
#include <util/box_utils.h>
#include <GLFW/glfw3.h>
#include <bx/timer.h>

namespace dirtbox {


class App {
    
public:
    void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height, const float scale)
    {
        Args args(_argc, _argv);

        m_width = _width;
        m_height = _height;
        m_ui_context.NotifyWindowSizeChanged({m_width, m_height});
        m_ui_context.SetUIScale(scale);
        m_debug = BGFX_DEBUG_NONE;
        m_reset_flags = BGFX_RESET_NONE;

        bgfx::Init init;
        init.type = args.m_type;
        init.vendorId = args.m_pciId;
        init.resolution.width = m_width;
        init.resolution.height = m_height;
        init.resolution.reset = m_reset_flags;
        bgfx::init(init);

        input::inputInit();
        utilInit();

        // Enable m_debug text.
        bgfx::setDebug(m_debug);

        // Set view 0 clear state.
        bgfx::setViewClear(0
            , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
            , 0x303030ff
            , 1.0f
            , 0
            );

        bgfx::setViewClear(1
            , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
            , 0x303030ff
            , 1.0f
            , 0
            );

        // Imgui.
        imguiCreate(18.0f * m_ui_context.GetUIScale());
        ImGui::GetStyle().ScaleAllSizes(m_ui_context.GetUIScale());

        m_timeOffset = bx::getHPCounter();

        m_oldWidth = 0;
        m_oldHeight = 0;
        m_oldReset = m_reset_flags;

        m_cam = std::make_unique<Camera>();
        m_cam->setPosition({ 0.0f, 0.5f, 0.0f });
        m_cam->setVerticalAngle(0);

        m_terrain_renderer = std::make_shared<TerrainRenderer>();
        m_terrain_renderer->init();

        m_terrain_manager = std::make_shared<terrain::TerrainManager>(vec2u{512, 512});
        m_resource_manager = std::make_shared<resource::ResourceManager>();

        ui_apps.emplace_back<std::shared_ptr<UIContent>>(std::make_shared<MenuBar>(m_ui_context));

    }



    bool update() {
        if (!processEvents(m_width, m_height))
        {
            m_mouseState = input::inputGetMouseState();
            int64_t now = bx::getHPCounter();
            static int64_t last = now;
            const int64_t frameTime = now - last;
            last = now;
            const double freq = double(bx::getHPFrequency() );
            const float deltaTime = float(frameTime / freq);

            // std::cout << std::to_string(m_mouseState.m_mx) << std::endl;

            imguiBeginFrame(
                m_mouseState.m_mx
                , m_mouseState.m_my
                , (m_mouseState.m_buttons[input::MouseButton::Left]   ? IMGUI_MBUT_LEFT   : 0)
                | (m_mouseState.m_buttons[input::MouseButton::Right]  ? IMGUI_MBUT_RIGHT  : 0)
                | (m_mouseState.m_buttons[input::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
                , m_mouseState.m_mz
                , uint16_t(m_width)
                , uint16_t(m_height)
                );

            // todo mismatched begin/end
            //showExampleDialog("");



            // GUI update
            for (auto& widget : ui_apps) {
                if (widget->enabled)
                    widget->OnGUIUpdate();
            }


            if (!ImGui::MouseOverArea() )
            {
                // Update camera.
                m_cam->update(deltaTime*0.01f, m_mouseState);
            }

            m_cam->getViewMtx(m_viewMtx);

            m_terrain_renderer->setTerrain(m_terrain_manager->getTerrain());

            m_terrain_renderer->setCameraInfo(m_viewMtx, m_width, m_height, 60.f);
            m_terrain_renderer->draw();

            imguiEndFrame();

            // Advance to next frame. Rendering thread will be kicked to
            // process submitted rendering primitives.
            uint32_t fid = bgfx::frame(false);

            Core::Get().FrameEvent.fire(fid);

            return true;
        }

        return false;
    }

    void showExampleDialog(const char* _errorText)
    {
        const std::string name = "NAME";
        const std::string description = "DESCRIPTION";

        ImGui::SetNextWindowPos(
            ImVec2(10.0f, 50.0f)
            , ImGuiCond_FirstUseEver
            );
        ImGui::SetNextWindowSize(
            ImVec2(300.0f, 210.0f)
            , ImGuiCond_FirstUseEver
            );

        ImGui::Begin(name.data());

        ImGui::TextWrapped("%s", description.data());

        // bx::StringView url = _app->getUrl();
        // if (!url.isEmpty() )
        // {
        //     ImGui::SameLine();
        //     if (ImGui::SmallButton(ICON_FA_LINK) )
        //     {
        //         openUrl(url);
        //     }
        //     else if (ImGui::IsItemHovered() )
        //     {
        //         char tmp[1024];
        //         bx::snprintf(tmp, BX_COUNTOF(tmp), "Documentation: %.*s", url.getLength(), url.getPtr() );
        //         ImGui::SetTooltip(tmp);
        //     }
        // }

        ImGui::Separator();

        if (NULL != _errorText)
        {
            const int64_t now  = bx::getHPCounter();
            const int64_t freq = bx::getHPFrequency();
            const float   time = float(now%freq)/float(freq);

            bool blink = time > 0.5f;

            ImGui::PushStyleColor(ImGuiCol_Text
                , blink
                ? ImVec4(1.0, 0.0, 0.0, 1.0)
                : ImVec4(1.0, 1.0, 1.0, 1.0)
                );
            ImGui::TextWrapped("%s", _errorText);
            ImGui::Separator();
            ImGui::PopStyleColor();
        }

        {
            const bgfx::Caps* caps = bgfx::getCaps();
            if (0 != (caps->supported & BGFX_CAPS_GRAPHICS_DEBUGGER) )
            {
                ImGui::SameLine();
                ImGui::Text("ICON_FA_SNOWFLAKE_O");
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f) );

            if (ImGui::Button(" Restart"))
            {
                //cmdExec("app restart");
            }

            // if (1 < entry::getNumApps() )
            // {
            //     ImGui::SameLine();
            //     if (ImGui::Button(" Prev") )
            //     {
            //         cmdExec("app restart prev");
            //     }

            //     ImGui::SameLine();
            //     if (ImGui::Button(" Next") )
            //     {
            //         cmdExec("app restart next");
            //     }
            // }

            ImGui::SameLine();
            if (ImGui::Button(" Exit"))
            {
                m_doExit = true;
            }

            ImGui::SameLine();
            m_showStats ^= ImGui::Button("ICON_FA_BAR_CHART");

            ImGui::PopStyleVar();
        }
    }

    /**
     * @brief pump events from window queue to input manager
     * 
     * @param _width 
     * @param _height 
     * @return true 
     * @return false 
     */
    bool processEvents(uint32_t& _width, uint32_t& _height)
    {
        bool doReset = false;

        bool mouseLock = input::inputIsMouseLocked();

        while (!windowEventsEmpty()) {
            struct SE { 
                std::unique_ptr<Event> m_ev; 
                SE() : m_ev{poll()} {} 
            } scopeEvent;

            const std::unique_ptr<Event>& ev = scopeEvent.m_ev;

            switch (ev->m_type)
            {
            case Event::Char:
                {
                    const CharEvent* chev = static_cast<const CharEvent*>(ev.get());
                    input::inputChar(chev->m_char);
                }
                break;

            case Event::Exit:
                return true;
            case Event::Mouse:
                {
                    const MouseEvent* mouse = static_cast<const MouseEvent*>(ev.get());

                    input::inputSetMousePos(mouse->m_mx, mouse->m_my, mouse->m_mz);
                    if (!mouse->m_move)
                    {
                        inputSetMouseButtonState(mouse->m_button, mouse->m_down);
                    }

                    // if (!mouseLock)
                    // {
                    //     _mouse.m_mx = mouse->m_mx;
                    //     _mouse.m_my = mouse->m_my;
                    //     _mouse.m_mz = mouse->m_mz;
                    //     if (!mouse->m_move)
                    //     {
                    //         _mouse.m_buttons[mouse->m_button] = mouse->m_down;
                    //     }
                    // }
                }
                break;

            case Event::Key:
                {
                    const KeyEvent* key = static_cast<const KeyEvent*>(ev.get());
                    input::inputSetKeyState(key->m_key, key->m_modifiers, key->m_down);
                }
                break;

            case Event::Size:
                {
                    const SizeEvent* size = static_cast<const SizeEvent*>(ev.get());
                    // win.m_width  = size->m_width;
                    // win.m_height = size->m_height;

                    _width  = size->m_width;
                    _height = size->m_height;
                    doReset = true; // force reset
                }
                break;

            case Event::Window:
                break;

            case Event::Suspend:
                break;

            case Event::DropFile:
                {
                    // const DropFileEvent* drop = static_cast<const DropFileEvent*>(ev);
                    // DBG("%s", drop->m_filePath.getCPtr() );
                }
                break;

            default:
                break;
            }

        }
        // TODO: also put in while loop above so there is an event per change, rather than the final state
        input::inputProcess();

        if (doReset)
        {
            bgfx::reset(_width, _height, m_reset_flags);
            input::inputSetMouseResolution(uint16_t(_width), uint16_t(_height));
        }

        m_width = _width;
        m_height = _height;

        return false;
    }

    int run() {
        bgfx::frame();

        setWindowSize(m_width, m_height);

        while (update())
        ;
        
        return shutdown();
    }

    int shutdown()
    {
        // Cleanup.
        imguiDestroy();

        return 0;
    }

    uint32_t m_width;
    uint32_t m_height;

    float m_viewMtx[16];
    float m_projMtx[16];

    uint32_t m_oldWidth;
    uint32_t m_oldHeight;
    uint32_t m_oldReset;

    input::MouseState m_mouseState;

    int64_t m_timeOffset;

    bool m_showStats = false;

    bool m_doExit = false;

    uint32_t m_reset_flags;
    uint32_t m_debug;

    std::unique_ptr<Camera> m_cam;
    std::vector<std::shared_ptr<UIContent>> ui_apps;

    UIContext m_ui_context;

    std::shared_ptr<TerrainRenderer> m_terrain_renderer;
    std::shared_ptr<terrain::TerrainManager> m_terrain_manager;
    std::shared_ptr<resource::ResourceManager> m_resource_manager;
};

} // dirtbox

static std::unique_ptr<dirtbox::App> app;

dirtbox::terrain::TerrainManager& dirtbox::GetTerrainManager() {
    return *app->m_terrain_manager.get();
}

dirtbox::TerrainRenderer& dirtbox::GetTerrainRenderer() {
    return *app->m_terrain_renderer.get();
}

dirtbox::resource::ResourceManager& GetResourceManager() {
    return *app->m_resource_manager.get();
}

int dirtbox::runApp(int argc, char *args[], uint32_t defaultWidth, uint32_t defaultHeight, const float scale) {
    int ret = 0;
    {
        app = std::make_unique<App>();
        app->init(argc, args, defaultWidth, defaultHeight, scale);
        int ret = app->run();
        app = {}; // app shutdown
    }
    // Shutdown bgfx last
    bgfx::shutdown();
    return ret;
}