#pragma once
#ifndef DIRTBOX_CAMERA_H_INCLUDE_GUARD
#define DIRTBOX_CAMERA_H_INCLUDE_GUARD

#include <bx/bx.h>
#include <bx/math.h>

#include <mk_input.h>

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)

namespace dirtbox {

struct Camera
{
    struct MouseCoords
    {
        int32_t m_mx;
        int32_t m_my;
    };

    Camera();
    ~Camera();

    void reset()
    {
        m_mouseNow.m_mx  = 0;
        m_mouseNow.m_my  = 0;
        m_mouseLast.m_mx = 0;
        m_mouseLast.m_my = 0;
        m_eye.x  =   0.0f;
        m_eye.y  =   0.0f;
        m_eye.z  = -35.0f;
        m_at.x   =   0.0f;
        m_at.y   =   0.0f;
        m_at.z   =  -1.0f;
        m_up.x   =   0.0f;
        m_up.y   =   1.0f;
        m_up.z   =   0.0f;
        m_horizontalAngle = 0.01f;
        m_verticalAngle = 0.0f;
        m_mouseSpeed = 0.0020f;
        m_gamepadSpeed = 0.04f;
        m_moveSpeed = 30.0f;
        m_keys = 0;
        m_mouseDown = false;
    }

    void forward() {
        setKeyState(CAMERA_KEY_FORWARD, true);
    }

    void backward() {
        setKeyState(CAMERA_KEY_BACKWARD, true);
    }

    void left() {
        setKeyState(CAMERA_KEY_LEFT, true);
    }

    void right() {
        setKeyState(CAMERA_KEY_RIGHT, true);
    }

    void up() {
        setKeyState(CAMERA_KEY_UP, true);
    }

    void down() {
        setKeyState(CAMERA_KEY_DOWN, true);
    }

    void setKeyState(uint8_t _key, bool _down)
    {
        m_keys &= ~_key;
        m_keys |= _down ? _key : 0;
    }

    void update(float _deltaTime, const input::MouseState& _mouseState)
    {
        if (!m_mouseDown)
        {
            m_mouseLast.m_mx = _mouseState.m_mx;
            m_mouseLast.m_my = _mouseState.m_my;
        }

        m_mouseDown = !!_mouseState.m_buttons[input::MouseButton::Right];

        if (m_mouseDown)
        {
            m_mouseNow.m_mx = _mouseState.m_mx;
            m_mouseNow.m_my = _mouseState.m_my;
        }

        if (m_mouseDown)
        {
            int32_t deltaX = m_mouseNow.m_mx - m_mouseLast.m_mx;
            int32_t deltaY = m_mouseNow.m_my - m_mouseLast.m_my;

            m_horizontalAngle += m_mouseSpeed * float(deltaX);
            m_verticalAngle   -= m_mouseSpeed * float(deltaY);

            m_mouseLast.m_mx = m_mouseNow.m_mx;
            m_mouseLast.m_my = m_mouseNow.m_my;
        }

        // input::GamepadHandle handle = { 0 };
        // m_horizontalAngle += m_gamepadSpeed * inputGetGamepadAxis(handle, input::GamepadAxis::RightX)/32768.0f;
        // m_verticalAngle   -= m_gamepadSpeed * inputGetGamepadAxis(handle, input::GamepadAxis::RightY)/32768.0f;
        // const int32_t gpx = inputGetGamepadAxis(handle, input::GamepadAxis::LeftX);
        // const int32_t gpy = inputGetGamepadAxis(handle, input::GamepadAxis::LeftY);
        // m_keys |= gpx < -16834 ? CAMERA_KEY_LEFT     : 0;
        // m_keys |= gpx >  16834 ? CAMERA_KEY_RIGHT    : 0;
        // m_keys |= gpy < -16834 ? CAMERA_KEY_FORWARD  : 0;
        // m_keys |= gpy >  16834 ? CAMERA_KEY_BACKWARD : 0;

        const bx::Vec3 direction =
        {
            bx::cos(m_verticalAngle) * bx::sin(m_horizontalAngle),
            bx::sin(m_verticalAngle),
            bx::cos(m_verticalAngle) * bx::cos(m_horizontalAngle),
        };

        const bx::Vec3 right =
        {
            bx::sin(m_horizontalAngle - bx::kPiHalf),
            0,
            bx::cos(m_horizontalAngle - bx::kPiHalf),
        };

        const bx::Vec3 up = bx::cross(right, direction);

        if (m_keys & CAMERA_KEY_FORWARD)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(direction, _deltaTime * m_moveSpeed);

            m_eye = bx::add(pos, tmp);
            setKeyState(CAMERA_KEY_FORWARD, false);
        }

        if (m_keys & CAMERA_KEY_BACKWARD)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(direction, _deltaTime * m_moveSpeed);

            m_eye = bx::sub(pos, tmp);
            setKeyState(CAMERA_KEY_BACKWARD, false);
        }

        if (m_keys & CAMERA_KEY_LEFT)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(right, _deltaTime * m_moveSpeed);

            m_eye = bx::add(pos, tmp);
            setKeyState(CAMERA_KEY_LEFT, false);
        }

        if (m_keys & CAMERA_KEY_RIGHT)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(right, _deltaTime * m_moveSpeed);

            m_eye = bx::sub(pos, tmp);
            setKeyState(CAMERA_KEY_RIGHT, false);
        }

        if (m_keys & CAMERA_KEY_UP)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(up, _deltaTime * m_moveSpeed);

            m_eye = bx::add(pos, tmp);
            setKeyState(CAMERA_KEY_UP, false);
        }

        if (m_keys & CAMERA_KEY_DOWN)
        {
            const bx::Vec3 pos = m_eye;
            const bx::Vec3 tmp = bx::mul(up, _deltaTime * m_moveSpeed);

            m_eye = bx::sub(pos, tmp);
            setKeyState(CAMERA_KEY_DOWN, false);
        }

        m_at = bx::add(m_eye, direction);
        m_up = bx::cross(right, direction);
    }

    void getViewMtx(float* _viewMtx)
    {
        bx::mtxLookAt(_viewMtx, bx::load<bx::Vec3>(&m_eye.x), bx::load<bx::Vec3>(&m_at.x), bx::load<bx::Vec3>(&m_up.x) );
    }

    void setPosition(const bx::Vec3& _pos)
    {
        m_eye = _pos;
    }

    void setVerticalAngle(float _verticalAngle)
    {
        m_verticalAngle = _verticalAngle;
    }

    void setHorizontalAngle(float _horizontalAngle)
    {
        m_horizontalAngle = _horizontalAngle;
    }

    MouseCoords m_mouseNow;
    MouseCoords m_mouseLast;

    bx::Vec3 m_eye;
    bx::Vec3 m_at;
    bx::Vec3 m_up;
    float m_horizontalAngle;
    float m_verticalAngle;

    float m_mouseSpeed;
    float m_gamepadSpeed;
    float m_moveSpeed;

    uint8_t m_keys;
    bool m_mouseDown;
};

} // namespace dirtbox

#endif // DIRTBOX_CAMERA_H_INCLUDE_GUARD