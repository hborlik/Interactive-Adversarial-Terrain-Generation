/**
 
 * 
 * @brief Input callback bindings manager. Must be supplied with input states
 * @version 0.1
 * @date 2021-01-15
 * 
 */
#pragma once
#ifndef DIRTBOX_INPUT_H_HEADER_GUARD
#define DIRTBOX_INPUT_H_HEADER_GUARD

#include <util/delegate.h>

#include <vector>
#include <string>
#include <memory>

namespace dirtbox::input {

struct MouseButton {
    enum Enum
    {
        Empty = 0,
        Left,
        Middle,
        Right,

        Count
    };
};

struct Modifier {
    enum Enum
    {
        Empty = 0,
        LeftAlt = 0x01,
        RightAlt = 0x02,
        LeftCtrl = 0x04,
        RightCtrl = 0x08,
        LeftShift = 0x10,
        RightShift = 0x20,
        LeftMeta = 0x40,
        RightMeta = 0x80,
    };
};

struct Key {
    enum Enum
    {
        Empty = 0,
        Esc,
        Return,
        Tab,
        Space,
        Backspace,
        Up,
        Down,
        Left,
        Right,
        Insert,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,
        Print,
        Plus,
        Minus,
        LeftBracket,
        RightBracket,
        Semicolon,
        Quote,
        Comma,
        Period,
        Slash,
        Backslash,
        Tilde,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        NumPad0,
        NumPad1,
        NumPad2,
        NumPad3,
        NumPad4,
        NumPad5,
        NumPad6,
        NumPad7,
        NumPad8,
        NumPad9,
        Key0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,
        KeyA,
        KeyB,
        KeyC,
        KeyD,
        KeyE,
        KeyF,
        KeyG,
        KeyH,
        KeyI,
        KeyJ,
        KeyK,
        KeyL,
        KeyM,
        KeyN,
        KeyO,
        KeyP,
        KeyQ,
        KeyR,
        KeyS,
        KeyT,
        KeyU,
        KeyV,
        KeyW,
        KeyX,
        KeyY,
        KeyZ,

        GamepadA,
        GamepadB,
        GamepadX,
        GamepadY,
        GamepadThumbL,
        GamepadThumbR,
        GamepadShoulderL,
        GamepadShoulderR,
        GamepadUp,
        GamepadDown,
        GamepadLeft,
        GamepadRight,
        GamepadBack,
        GamepadStart,
        GamepadGuide,

        Count
    };

    Key() = delete;
};

const char* getName(Key::Enum _key);

struct MouseState
{
    MouseState()
        : m_mx(0)
        , m_my(0)
        , m_mz(0)
    {
        for (uint32_t ii = 0; ii < MouseButton::Count; ++ii)
        {
            m_buttons[ii] = MouseButton::Empty;
        }
    }

    int32_t m_mx;
    int32_t m_my;
    int32_t m_mz;
    std::array<uint8_t, MouseButton::Count> m_buttons;
};

using InputBindingFn = util::delegate<void()>;

struct InputBinding
{
    void set(Key::Enum _key, uint8_t _modifiers, uint8_t _flags, InputBindingFn _fn)
    {
        m_key = _key;
        m_modifiers = _modifiers;
        m_flags     = _flags;
        m_fn        = _fn;
    }

    void end()
    {
        m_key = Key::Enum::Empty;
        m_modifiers = Modifier::Empty;
        m_flags     = 0;
        m_fn        = nullptr;
    }

    InputBinding(
        Key::Enum m_key,
        uint8_t m_modifiers,
        uint8_t m_flags,
        InputBindingFn m_fn) : 
        m_key{m_key},
        m_modifiers{m_modifiers},
        m_flags{m_flags},
        m_fn{m_fn}
        {}

    Key::Enum m_key;
    uint8_t m_modifiers;
    uint8_t m_flags;
    InputBindingFn m_fn;
};

///
void inputInit();

///
void inputShutdown();

///
void inputAddBindings(const std::string& _name, const std::vector<std::shared_ptr<InputBinding>>& _bindings);

///
void inputRemoveBindings(const std::string& name);

///
void inputProcess();

///
void inputSetKeyState(Key::Enum  _key, uint8_t _modifiers, bool _down);

///
bool inputGetKeyState(Key::Enum _key, uint8_t* _modifiers = nullptr);

///
uint8_t inputGetModifiersState();

/// Adds single UTF-8 encoded character into input buffer.
void inputChar(uint32_t character);

/// Returns single UTF-8 encoded character from input buffer.
const uint32_t inputGetChar();

/// Flush internal input buffer.
void inputCharFlush();

///
void inputSetMouseResolution(uint16_t _width, uint16_t _height);

///
void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz);

///
void inputSetMouseButtonState(MouseButton::Enum _button, uint8_t _state);

///
void inputSetMouseLock(bool _lock);

///
void inputGetMouse(float _mouse[3]);

///
bool inputIsMouseLocked();


MouseState inputGetMouseState();

} // dirtbox::input

#endif // DIRTBOX_INPUT_H_HEADER_GUARD