/**
 
 * @author 
 * @brief window private header based on bgfx common example code for input processing
 * @version 0.1
 * @date 2021-01-21
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_WINDOW_P_H_HEADER_GUARD
#define DIRTBOX_WINDOW_P_H_HEADER_GUARD

#include <filesystem>
#include <memory>

#include <mk_input.h>
#include <util/sharedqueue.h>

namespace dirtbox {

int main(int _argc, const char* const* _argv);

char keyToAscii(input::Key::Enum _key, uint8_t _modifiers);

struct Event
{
    enum Enum
    {
        Char,
        Exit,
        Key,
        Mouse,
        Size,
        Window,
        Suspend,
        DropFile,
    };

    Event(Enum _type)
        : m_type(_type)
    {
    }


    Event::Enum m_type;
};

struct CharEvent : public Event
{
    CharEvent() : Event {Event::Char} {}

    uint32_t m_char;
};

struct KeyEvent : public Event
{
    KeyEvent() : Event{Event::Key} {}

    input::Key::Enum m_key;
    uint8_t m_modifiers;
    bool m_down;
};

struct MouseEvent : public Event
{
    MouseEvent() : Event{Event::Mouse} {}

    int32_t m_mx;
    int32_t m_my;
    int32_t m_mz;
    input::MouseButton::Enum m_button;
    bool m_down;
    bool m_move;
};

struct SizeEvent : public Event
{
    SizeEvent() : Event{Event::Size} {}

    uint32_t m_width;
    uint32_t m_height;
};

struct WindowEvent : public Event
{
    WindowEvent() : Event{Event::Window} {}

    void* m_nwh;
};

struct DropFileEvent : public Event
{
    DropFileEvent() : Event{Event::DropFile} {}

    std::filesystem::path m_filePath;
};

std::unique_ptr<Event> poll();

class EventQueue
{
public:
    EventQueue()
        : m_queue{}
    {
    }

    void postCharEvent(uint32_t _char)
    {
        auto ev = std::make_unique<CharEvent>();
        ev->m_char = _char;
        m_queue.push_back(std::move(ev));
    }

    void postExitEvent()
    {
        auto ev = std::make_unique<Event>(Event::Exit);
        m_queue.push_back(std::move(ev));
    }

    void postKeyEvent(input::Key::Enum _key, uint8_t _modifiers, bool _down)
    {
        auto ev = std::make_unique<KeyEvent>();
        ev->m_key       = _key;
        ev->m_modifiers = _modifiers;
        ev->m_down      = _down;
        m_queue.push_back(std::move(ev));
    }

    void postMouseEvent(int32_t _mx, int32_t _my, int32_t _mz)
    {
        auto ev = std::make_unique<MouseEvent>();
        ev->m_mx     = _mx;
        ev->m_my     = _my;
        ev->m_mz     = _mz;
        ev->m_button = input::MouseButton::Empty;
        ev->m_down   = false;
        ev->m_move   = true;
        m_queue.push_back(std::move(ev));
    }

    void postMouseEvent(int32_t _mx, int32_t _my, int32_t _mz, input::MouseButton::Enum _button, bool _down)
    {
        auto ev = std::make_unique<MouseEvent>();
        ev->m_mx     = _mx;
        ev->m_my     = _my;
        ev->m_mz     = _mz;
        ev->m_button = _button;
        ev->m_down   = _down;
        ev->m_move   = false;
        m_queue.push_back(std::move(ev));
    }

    void postSizeEvent(uint32_t _width, uint32_t _height)
    {
        auto ev = std::make_unique<SizeEvent>();
        ev->m_width  = _width;
        ev->m_height = _height;
        m_queue.push_back(std::move(ev));
    }

    void postWindowEvent(void* _nwh = NULL)
    {
        auto ev = std::make_unique<WindowEvent>();
        ev->m_nwh = _nwh;
        m_queue.push_back(std::move(ev));
    }

    void postDropFileEvent(const std::filesystem::path& _filePath)
    {
        auto ev = std::make_unique<DropFileEvent>();
        ev->m_filePath = _filePath;
        m_queue.push_back(std::move(ev));
    }

    std::unique_ptr<Event> poll()
    {
        if (m_queue.empty())
            return {nullptr};
        return m_queue.pop_front();
    }

    bool queueEmpty() {
        return m_queue.empty();
    }

private:
    util::SharedQueue<std::unique_ptr<Event>> m_queue;
};

} // namespace dirtbox

#endif // DIRTBOX_WINDOW_P_H_HEADER_GUARD