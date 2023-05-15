#include <array>
#include <iostream>
#include <thread>
#include <mutex>

#include <GLFW/glfw3.h>
#if GLFW_VERSION_MINOR < 2
#	error "GLFW 3.2 or later is required"
#endif // GLFW_VERSION_MINOR < 2

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>


#include <window.h>
#include <window_events.h>
#include <app.h>
#include <mk_input.h>
#include <util/sharedqueue.h>

#include <bgfx/platform.h>

using namespace dirtbox::input;
using namespace std;

namespace dirtbox {

static uint8_t translateKeyModifiers(int _glfw)
{
    uint8_t modifiers = 0;

    if (_glfw & GLFW_MOD_ALT)
    {
        modifiers |= Modifier::LeftAlt;
    }

    if (_glfw & GLFW_MOD_CONTROL)
    {
        modifiers |= Modifier::LeftCtrl;
    }

    if (_glfw & GLFW_MOD_SUPER)
    {
        modifiers |= Modifier::LeftMeta;
    }

    if (_glfw & GLFW_MOD_SHIFT)
    {
        modifiers |= Modifier::LeftShift;
    }

    return modifiers;
}

// key translations map
static array<Key::Enum, GLFW_KEY_LAST + 1> s_translateKey;

static Key::Enum translateKey(int _key)
{
    return s_translateKey[_key];
}

static MouseButton::Enum translateMouseButton(int _button)
{
    if (_button == GLFW_MOUSE_BUTTON_LEFT)
    {
        return MouseButton::Left;
    }
    else if (_button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        return MouseButton::Right;
    }

    return MouseButton::Middle;
}

static void glfwSetWindow(GLFWwindow* _window)
{
    bgfx::PlatformData pd;
    pd.ndt          = glfwGetX11Display();
    pd.nwh          = (void*)glfwGetX11Window(_window);
    pd.context      = NULL;
    pd.backBuffer   = NULL;
    pd.backBufferDS = NULL;
    bgfx::setPlatformData(pd);
}

enum class MsgType
{
    //GLFW_WINDOW_CREATE,
    GLFW_WINDOW_DESTROY,
    GLFW_WINDOW_SET_TITLE,
    GLFW_WINDOW_SET_POS,
    GLFW_WINDOW_SET_SIZE,
    GLFW_WINDOW_TOGGLE_FRAME,
    GLFW_WINDOW_TOGGLE_FULL_SCREEN,
    GLFW_WINDOW_MOUSE_LOCK
};

struct Msg
{
    Msg(MsgType _type)
        : m_type{_type}
        , m_x{0}
        , m_y{0}
        , m_width{0}
        , m_height{0}
        , m_value{false}
    {
    }

    MsgType     m_type;
    int32_t     m_x;
    int32_t     m_y;
    uint32_t    m_width;
    uint32_t    m_height;
    uint32_t    m_flags;
    bool	    m_value;
    string      m_title;
};

static void errorCb(int _error, const char* _description)
{
    cout << "GLFW error " + to_string(_error) + _description << endl;
}

// Based on cutef8 by Jeff Bezanson (Public Domain)
static uint8_t encodeUTF8(uint8_t _chars[4], uint32_t _scancode)
{
    uint8_t length = 0;

    if (_scancode < 0x80)
    {
        _chars[length++] = (char) _scancode;
    }
    else if (_scancode < 0x800)
    {
        _chars[length++] =  (_scancode >>  6)         | 0xc0;
        _chars[length++] =  (_scancode        & 0x3f) | 0x80;
    }
    else if (_scancode < 0x10000)
    {
        _chars[length++] =  (_scancode >> 12)         | 0xe0;
        _chars[length++] = ((_scancode >>  6) & 0x3f) | 0x80;
        _chars[length++] =  (_scancode        & 0x3f) | 0x80;
    }
    else if (_scancode < 0x110000)
    {
        _chars[length++] =  (_scancode >> 18)         | 0xf0;
        _chars[length++] = ((_scancode >> 12) & 0x3f) | 0x80;
        _chars[length++] = ((_scancode >>  6) & 0x3f) | 0x80;
        _chars[length++] =  (_scancode        & 0x3f) | 0x80;
    }

    return length;
}

/**
 * @brief Multithreaded event processing and main application
 * App runs on a separate thread from the event processing loop.
 * 
 */
class Context {
public:
    Context() {
        s_translateKey.fill(Key::Empty);
        s_translateKey[GLFW_KEY_ESCAPE]       = Key::Esc;
        s_translateKey[GLFW_KEY_ENTER]        = Key::Return;
        s_translateKey[GLFW_KEY_TAB]          = Key::Tab;
        s_translateKey[GLFW_KEY_BACKSPACE]    = Key::Backspace;
        s_translateKey[GLFW_KEY_SPACE]        = Key::Space;
        s_translateKey[GLFW_KEY_UP]           = Key::Up;
        s_translateKey[GLFW_KEY_DOWN]         = Key::Down;
        s_translateKey[GLFW_KEY_LEFT]         = Key::Left;
        s_translateKey[GLFW_KEY_RIGHT]        = Key::Right;
        s_translateKey[GLFW_KEY_PAGE_UP]      = Key::PageUp;
        s_translateKey[GLFW_KEY_PAGE_DOWN]    = Key::PageDown;
        s_translateKey[GLFW_KEY_HOME]         = Key::Home;
        s_translateKey[GLFW_KEY_END]          = Key::End;
        s_translateKey[GLFW_KEY_PRINT_SCREEN] = Key::Print;
        s_translateKey[GLFW_KEY_KP_ADD]       = Key::Plus;
        s_translateKey[GLFW_KEY_EQUAL]        = Key::Plus;
        s_translateKey[GLFW_KEY_KP_SUBTRACT]  = Key::Minus;
        s_translateKey[GLFW_KEY_MINUS]        = Key::Minus;
        s_translateKey[GLFW_KEY_COMMA]        = Key::Comma;
        s_translateKey[GLFW_KEY_PERIOD]       = Key::Period;
        s_translateKey[GLFW_KEY_SLASH]        = Key::Slash;
        s_translateKey[GLFW_KEY_F1]           = Key::F1;
        s_translateKey[GLFW_KEY_F2]           = Key::F2;
        s_translateKey[GLFW_KEY_F3]           = Key::F3;
        s_translateKey[GLFW_KEY_F4]           = Key::F4;
        s_translateKey[GLFW_KEY_F5]           = Key::F5;
        s_translateKey[GLFW_KEY_F6]           = Key::F6;
        s_translateKey[GLFW_KEY_F7]           = Key::F7;
        s_translateKey[GLFW_KEY_F8]           = Key::F8;
        s_translateKey[GLFW_KEY_F9]           = Key::F9;
        s_translateKey[GLFW_KEY_F10]          = Key::F10;
        s_translateKey[GLFW_KEY_F11]          = Key::F11;
        s_translateKey[GLFW_KEY_F12]          = Key::F12;
        s_translateKey[GLFW_KEY_KP_0]         = Key::NumPad0;
        s_translateKey[GLFW_KEY_KP_1]         = Key::NumPad1;
        s_translateKey[GLFW_KEY_KP_2]         = Key::NumPad2;
        s_translateKey[GLFW_KEY_KP_3]         = Key::NumPad3;
        s_translateKey[GLFW_KEY_KP_4]         = Key::NumPad4;
        s_translateKey[GLFW_KEY_KP_5]         = Key::NumPad5;
        s_translateKey[GLFW_KEY_KP_6]         = Key::NumPad6;
        s_translateKey[GLFW_KEY_KP_7]         = Key::NumPad7;
        s_translateKey[GLFW_KEY_KP_8]         = Key::NumPad8;
        s_translateKey[GLFW_KEY_KP_9]         = Key::NumPad9;
        s_translateKey[GLFW_KEY_0]            = Key::Key0;
        s_translateKey[GLFW_KEY_1]            = Key::Key1;
        s_translateKey[GLFW_KEY_2]            = Key::Key2;
        s_translateKey[GLFW_KEY_3]            = Key::Key3;
        s_translateKey[GLFW_KEY_4]            = Key::Key4;
        s_translateKey[GLFW_KEY_5]            = Key::Key5;
        s_translateKey[GLFW_KEY_6]            = Key::Key6;
        s_translateKey[GLFW_KEY_7]            = Key::Key7;
        s_translateKey[GLFW_KEY_8]            = Key::Key8;
        s_translateKey[GLFW_KEY_9]            = Key::Key9;
        s_translateKey[GLFW_KEY_A]            = Key::KeyA;
        s_translateKey[GLFW_KEY_B]            = Key::KeyB;
        s_translateKey[GLFW_KEY_C]            = Key::KeyC;
        s_translateKey[GLFW_KEY_D]            = Key::KeyD;
        s_translateKey[GLFW_KEY_E]            = Key::KeyE;
        s_translateKey[GLFW_KEY_F]            = Key::KeyF;
        s_translateKey[GLFW_KEY_G]            = Key::KeyG;
        s_translateKey[GLFW_KEY_H]            = Key::KeyH;
        s_translateKey[GLFW_KEY_I]            = Key::KeyI;
        s_translateKey[GLFW_KEY_J]            = Key::KeyJ;
        s_translateKey[GLFW_KEY_K]            = Key::KeyK;
        s_translateKey[GLFW_KEY_L]            = Key::KeyL;
        s_translateKey[GLFW_KEY_M]            = Key::KeyM;
        s_translateKey[GLFW_KEY_N]            = Key::KeyN;
        s_translateKey[GLFW_KEY_O]            = Key::KeyO;
        s_translateKey[GLFW_KEY_P]            = Key::KeyP;
        s_translateKey[GLFW_KEY_Q]            = Key::KeyQ;
        s_translateKey[GLFW_KEY_R]            = Key::KeyR;
        s_translateKey[GLFW_KEY_S]            = Key::KeyS;
        s_translateKey[GLFW_KEY_T]            = Key::KeyT;
        s_translateKey[GLFW_KEY_U]            = Key::KeyU;
        s_translateKey[GLFW_KEY_V]            = Key::KeyV;
        s_translateKey[GLFW_KEY_W]            = Key::KeyW;
        s_translateKey[GLFW_KEY_X]            = Key::KeyX;
        s_translateKey[GLFW_KEY_Y]            = Key::KeyY;
        s_translateKey[GLFW_KEY_Z]            = Key::KeyZ;
    }

    int run(int argc, char *args[]) {
        glfwSetErrorCallback(errorCb);

        if (!glfwInit() )
        {
            clog << "glfwInit failed!" << endl;
            return EXIT_FAILURE;
        }

        //glfwSetJoystickCallback(joystickCb);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // query system info
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        int width_mm, height_mm, defaultWidth, defaultHeight;
        float scale;
        glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);
        glfwGetMonitorContentScale(monitor, &scale, nullptr);
        glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &defaultWidth, &defaultHeight);
        if (width_mm == 0 || height_mm == 0) {
            clog << "glfwGetMonitorPhysicalSize failed!" << endl;
            width_mm = defaultWidth;
            height_mm = defaultHeight;
        }

        m_window = glfwCreateWindow(defaultWidth
            , defaultHeight
            , "bgfx"
            , NULL
            , NULL
            );

        if (!m_window)
        {
            clog << "glfwCreateWindow failed!" << endl;
            glfwTerminate();
            return EXIT_FAILURE;
        }
        

        // setup window callbacks
        glfwSetKeyCallback(m_window, keyCb);
        glfwSetCharCallback(m_window, charCb);
        glfwSetScrollCallback(m_window, scrollCb);
        glfwSetCursorPosCallback(m_window, cursorPosCb);
        glfwSetMouseButtonCallback(m_window, mouseButtonCb);
        glfwSetWindowSizeCallback(m_window, windowSizeCb);
        glfwSetDropCallback(m_window, dropFileCb);

        glfwSetWindow(m_window);
        m_eventQueue.postSizeEvent(defaultWidth, defaultHeight);

        // start application thread
        m_app_thread = thread{runApp, argc, args, (uint32_t)defaultWidth, (uint32_t)defaultHeight, scale};

        // window control messages processing
        while (nullptr != m_window && !glfwWindowShouldClose(m_window))
        {
            glfwWaitEvents();

            while (!m_msgs.empty())
            {
                auto msg = m_msgs.pop_front();
                switch (msg->m_type)
                {
                case MsgType::GLFW_WINDOW_DESTROY:
                    {
                        GLFWwindow* window = m_window;
                        m_eventQueue.postWindowEvent();
                        glfwDestroyWindow(window);
                        m_window = nullptr;
                    }
                    break;

                case MsgType::GLFW_WINDOW_SET_TITLE:
                    {
                        GLFWwindow* window = m_window;
                        glfwSetWindowTitle(window, msg->m_title.c_str());
                    }
                    break;

                case MsgType::GLFW_WINDOW_SET_POS:
                    {
                        GLFWwindow* window = m_window;
                        glfwSetWindowPos(window, msg->m_x, msg->m_y);
                    }
                    break;

                case MsgType::GLFW_WINDOW_SET_SIZE:
                    {
                        GLFWwindow* window = m_window;
                        glfwSetWindowSize(window, msg->m_width, msg->m_height);
                    }
                    break;

                case MsgType::GLFW_WINDOW_TOGGLE_FRAME:
                    {
                        // Wait for glfwSetWindowDecorated to exist
                    }
                    break;

                case MsgType::GLFW_WINDOW_TOGGLE_FULL_SCREEN:
                    {
                        GLFWwindow* window = m_window;
                        if (glfwGetWindowMonitor(window) )
                        {
                            glfwSetWindowMonitor(window
                                , NULL
                                , m_oldX
                                , m_oldY
                                , m_oldWidth
                                , m_oldHeight
                                , 0
                                );
                        }
                        else
                        {
                            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                            if (NULL != monitor)
                            {
                                glfwGetWindowPos(window, &m_oldX, &m_oldY);
                                glfwGetWindowSize(window, &m_oldWidth, &m_oldHeight);

                                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                                glfwSetWindowMonitor(window
                                    , monitor
                                    , 0
                                    , 0
                                    , mode->width
                                    , mode->height
                                    , mode->refreshRate
                                    );
                            }

                        }
                    }
                    break;

                case MsgType::GLFW_WINDOW_MOUSE_LOCK:
                    {
                        GLFWwindow* window = m_window;
                        if (msg->m_value)
                        {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        }
                        else
                        {
                            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        }
                    }
                    break;
                }
            }
        }

        // shutdown
        m_eventQueue.postExitEvent();
        m_app_thread.join();

        glfwDestroyWindow(m_window);
        glfwTerminate();

        return EXIT_SUCCESS;
    }

    util::SharedQueue<unique_ptr<Msg>> m_msgs;

    EventQueue m_eventQueue;
private:
    static void keyCb(GLFWwindow* _window, int32_t _key, int32_t _scancode, int32_t _action, int32_t _mods);
    static void charCb(GLFWwindow* _window, uint32_t _scancode);
    static void scrollCb(GLFWwindow* _window, double _dx, double _dy);
    static void cursorPosCb(GLFWwindow* _window, double _mx, double _my);
    static void mouseButtonCb(GLFWwindow* _window, int32_t _button, int32_t _action, int32_t _mods);
    static void windowSizeCb(GLFWwindow* _window, int32_t _width, int32_t _height);
    static void dropFileCb(GLFWwindow* _window, int32_t _count, const char** _filePaths);

    thread m_app_thread;


    GLFWwindow* m_window;

    int32_t m_oldX;
    int32_t m_oldY;
    int32_t m_oldWidth;
    int32_t m_oldHeight;

    double m_scrollPos;
};

static unique_ptr<Context> s_ctx;

void Context::keyCb(GLFWwindow* _window, int32_t _key, int32_t _scancode, int32_t _action, int32_t _mods)
{
    if (_key == GLFW_KEY_UNKNOWN)
    {
        return;
    }
    int mods = translateKeyModifiers(_mods);
    Key::Enum key = translateKey(_key);
    bool down = (_action == GLFW_PRESS || _action == GLFW_REPEAT);
    s_ctx->m_eventQueue.postKeyEvent(key, mods, down);
}

void Context::charCb(GLFWwindow* _window, uint32_t _scancode)
{
    s_ctx->m_eventQueue.postCharEvent(_scancode);
}

void Context::scrollCb(GLFWwindow* _window, double _dx, double _dy)
{
    double mx, my;
    glfwGetCursorPos(_window, &mx, &my);
    s_ctx->m_scrollPos += _dy;
    s_ctx->m_eventQueue.postMouseEvent(
        (int32_t) mx,
        (int32_t) my,
        (int32_t) s_ctx->m_scrollPos
        );
}

void Context::cursorPosCb(GLFWwindow* _window, double _mx, double _my)
{
    s_ctx->m_eventQueue.postMouseEvent(
        (int32_t) _mx,
        (int32_t) _my,
        (int32_t) s_ctx->m_scrollPos
        );
}

void Context::mouseButtonCb(GLFWwindow* _window, int32_t _button, int32_t _action, int32_t _mods)
{
    bool down = _action == GLFW_PRESS;
    double mx, my;
    glfwGetCursorPos(_window, &mx, &my);
    s_ctx->m_eventQueue.postMouseEvent(
        (int32_t) mx,
        (int32_t) my,
        (int32_t) s_ctx->m_scrollPos,
        translateMouseButton(_button),
        down
        );
}

void Context::windowSizeCb(GLFWwindow* _window, int32_t _width, int32_t _height)
{
    s_ctx->m_eventQueue.postSizeEvent(_width, _height);
}

void Context::dropFileCb(GLFWwindow* _window, int32_t _count, const char** _filePaths)
{
    for (int32_t ii = 0; ii < _count; ++ii)
    {
        s_ctx->m_eventQueue.postDropFileEvent(_filePaths[ii]);
    }
}

void destroyWindow()
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_DESTROY);
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

void setWindowPos(int32_t _x, int32_t _y)
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_SET_POS);
    msg->m_x = _x;
    msg->m_y = _y;
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

void setWindowSize(uint32_t _width, uint32_t _height)
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_SET_SIZE);
    msg->m_width = _width;
    msg->m_height = _height;
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

void setWindowTitle(const char* _title)
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_SET_TITLE);
    msg->m_title = _title;
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

void toggleFullscreen()
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_TOGGLE_FULL_SCREEN);
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

void setMouseLock(bool _lock)
{
    auto msg = make_unique<Msg>(MsgType::GLFW_WINDOW_MOUSE_LOCK);
    msg->m_value = _lock;
    s_ctx->m_msgs.push_back(move(msg));
    glfwPostEmptyEvent();
}

std::unique_ptr<Event> poll()
{
    return s_ctx->m_eventQueue.poll();
}

bool windowEventsEmpty()
{
    return s_ctx->m_eventQueue.queueEmpty();
}

int run(int argc, char *args[]) {
    s_ctx = make_unique<Context>();
    return s_ctx->run(argc, args);
}

} // namespace dirtbox