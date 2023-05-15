/*
 * Copyright 2010-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 * 
 * 
 * modified to use c++ dynamic structures.
 */


#include <bx/allocator.h>
#include <string>
#include <unordered_map>
#include <array>

#include <mk_input.h>
#include <window.h>

#include <bx/ringbuffer.h>

using namespace dirtbox::input;
using namespace std;

namespace dirtbox {

struct InputMouse
{
    InputMouse()
        : m_width{1280}
        , m_height{720}
        , m_wheelDelta{120}
        , m_lock{false}
    {
    }

    void reset()
    {
        if (m_lock)
        {
            m_norm[0] = 0.0f;
            m_norm[1] = 0.0f;
            m_norm[2] = 0.0f;
        }
    }

    void setResolution(uint16_t _width, uint16_t _height)
    {
        m_width = _width;
        m_height = _height;
    }

    void setPos(int32_t _mx, int32_t _my, int32_t _mz)
    {
        m_absolute[0] = _mx;
        m_absolute[1] = _my;
        m_absolute[2] = _mz;
        m_norm[0] = float(_mx)/float(m_width);
        m_norm[1] = float(_my)/float(m_height);
        m_norm[2] = float(_mz)/float(m_wheelDelta);
    }

    void setButtonState(MouseButton::Enum _button, uint8_t _state)
    {
        m_buttons[_button] = _state;
    }

    int32_t m_absolute[3];
    float m_norm[3];
    int32_t m_wheel;
    std::array<uint8_t, MouseButton::Count> m_buttons;
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_wheelDelta;
    bool m_lock;
};

struct InputKeyboard
{
    InputKeyboard()
        : m_ring{m_char.size() - 1}
    {
    }

    void reset()
    {
        m_key.fill(0);
        m_once.fill(0);
    }

    static uint32_t encodeKeyState(uint8_t _modifiers, bool _down)
    {
        uint32_t state = 0;
        state |= uint32_t(_down ? _modifiers : 0)<<16;
        state |= uint32_t(_down)<<8;
        return state;
    }

    static bool decodeKeyState(uint32_t _state, uint8_t& _modifiers)
    {
        _modifiers = (_state>>16)&0xff;
        return 0 != ( (_state>> 8)&0xff);
    }

    void setKeyState(Key::Enum _key, uint8_t _modifiers, bool _down)
    {
        m_key[_key] = encodeKeyState(_modifiers, _down);
        m_once[_key] = false;
    }

    bool getKeyState(Key::Enum _key, uint8_t* _modifiers)
    {
        uint8_t modifiers;
        _modifiers = NULL == _modifiers ? &modifiers : _modifiers;

        return decodeKeyState(m_key[_key], *_modifiers);
    }

    uint8_t getModifiersState()
    {
        uint8_t modifiers = 0;
        for (uint32_t ii = 0; ii < Key::Count; ++ii)
        {
            modifiers |= (m_key[ii]>>16)&0xff;
        }
        return modifiers;
    }

    void pushChar(uint32_t _char)
    {
        uint32_t len = m_ring.reserve(1);
        if (len != 1) {
            popChar();
        }

        m_char[m_ring.m_current] = _char;
        m_ring.commit(1);
    }

    uint32_t popChar()
    {
        if (0 < m_ring.available() )
        {
            uint32_t utf = m_char[m_ring.m_read];
            m_ring.consume(1);
            return utf;
        }
        return 0;
    }

    void charFlush()
    {
        m_ring.m_current = 0;
        m_ring.m_write   = 0;
        m_ring.m_read    = 0;
    }

    // key states
    array<uint32_t, 256> m_key;
    array<bool, 256> m_once;

    // input character buffer
    bx::RingBufferControl m_ring;
    array<uint32_t, 256> m_char;
};

struct Input
{
    using BindingPtr = shared_ptr<InputBinding>;
    using InputBindingMap = unordered_map<string, vector<BindingPtr>>;

    InputBindingMap m_inputBindingsMap;
    InputKeyboard m_keyboard;
    InputMouse m_mouse;


    Input()
    {
        reset();
    }

    ~Input()
    {
    }

    void addBindings(const string& _name, const vector<BindingPtr>& _bindings)
    {
        m_inputBindingsMap.insert(make_pair(_name, _bindings));
    }

    void removeBindings(const string& _name)
    {
        auto it = m_inputBindingsMap.find(_name);
        if (it != m_inputBindingsMap.end() )
        {
            m_inputBindingsMap.erase(it);
        }
    }

    /**
     * @brief process input bindings based on current key and mouse states
     * 
     * @param _bindings 
     */
    void process(const vector<BindingPtr>& _bindings)
    {
        for (auto binding_it = _bindings.begin(); binding_it != _bindings.end(); ++binding_it)
        {
            auto& binding = *binding_it;
            uint8_t modifiers;
            bool down = InputKeyboard::decodeKeyState(m_keyboard.m_key[binding->m_key], modifiers);

            if (binding->m_flags == 1)
            {
                if (down)
                {
                    if (modifiers == binding->m_modifiers
                    &&  !m_keyboard.m_once[binding->m_key])
                    {
                        if (!binding->m_fn.isNull())
                        {
                            binding->m_fn();
                        }
                        m_keyboard.m_once[binding->m_key] = true;
                    }
                }
                else
                {
                    m_keyboard.m_once[binding->m_key] = false;
                }
            }
            else
            {
                if (down
                &&  modifiers == binding->m_modifiers)
                {
                    if (!binding->m_fn.isNull())
                    {
                        binding->m_fn();
                    }
                }
            }
        }
    }

    void process()
    {
        for (auto it = m_inputBindingsMap.begin(); it != m_inputBindingsMap.end(); ++it)
        {
            process(it->second);
        }
    }

    void reset()
    {
        m_mouse.reset();
        m_keyboard.reset();
    }
};

static unique_ptr<Input> s_input;

void input::inputInit()
{
    s_input = make_unique<Input>();
}

void input::inputShutdown()
{
    
}

void input::inputAddBindings(const string& _name, const vector<Input::BindingPtr>& _bindings)
{
    if(s_input)
        s_input->addBindings(_name, _bindings);
}

void input::inputRemoveBindings(const string& _name)
{
    if(s_input)
        s_input->removeBindings(_name);
}

void input::inputProcess()
{
    if(s_input)
        s_input->process();
}

void input::inputSetMouseResolution(uint16_t _width, uint16_t _height)
{
    if(s_input)
        s_input->m_mouse.setResolution(_width, _height);
}

void input::inputSetKeyState(Key::Enum _key, uint8_t _modifiers, bool _down)
{
    if(s_input)
        s_input->m_keyboard.setKeyState(_key, _modifiers, _down);
}

bool input::inputGetKeyState(Key::Enum _key, uint8_t* _modifiers)
{
    if(s_input)
        return s_input->m_keyboard.getKeyState(_key, _modifiers);
    return false;
}

uint8_t input::inputGetModifiersState()
{
    if(s_input)
        return s_input->m_keyboard.getModifiersState();
    return 0;
}

void input::inputChar(uint32_t _scancode)
{
    if(s_input)
        s_input->m_keyboard.pushChar(_scancode);
}

const uint32_t input::inputGetChar()
{
    if(s_input)
        return s_input->m_keyboard.popChar();
    return 0;
}

void input::inputCharFlush()
{
    if(s_input)
        s_input->m_keyboard.charFlush();
}

void input::inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz)
{
    if(s_input)
        s_input->m_mouse.setPos(_mx, _my, _mz);
}

void input::inputSetMouseButtonState(MouseButton::Enum _button, uint8_t _state)
{
    if(s_input)
        s_input->m_mouse.setButtonState(_button, _state);
}

void input::inputGetMouse(float _mouse[3])
{
    if(!s_input)
        return;

    _mouse[0] = s_input->m_mouse.m_norm[0];
    _mouse[1] = s_input->m_mouse.m_norm[1];
    _mouse[2] = s_input->m_mouse.m_norm[2];
    s_input->m_mouse.m_norm[0] = 0.0f;
    s_input->m_mouse.m_norm[1] = 0.0f;
    s_input->m_mouse.m_norm[2] = 0.0f;
}

bool input::inputIsMouseLocked()
{
    if(s_input)
        return s_input->m_mouse.m_lock;
    return false;
}

void input::inputSetMouseLock(bool _lock)
{
    if(!s_input)
        return;

    if (s_input->m_mouse.m_lock != _lock)
    {
        s_input->m_mouse.m_lock = _lock;
        dirtbox::setMouseLock(_lock);
        if (_lock)
        {
            s_input->m_mouse.m_norm[0] = 0.0f;
            s_input->m_mouse.m_norm[1] = 0.0f;
            s_input->m_mouse.m_norm[2] = 0.0f;
        }
    }
}

MouseState input::inputGetMouseState() {
    MouseState ret{};
    if(s_input) {
        auto& mouse = s_input->m_mouse;
        ret.m_mx = mouse.m_absolute[0];
        ret.m_my = mouse.m_absolute[1];
        ret.m_mz = mouse.m_absolute[2];
        ret.m_buttons = mouse.m_buttons;
    }
    return ret;
}

} // namespace dirtbox