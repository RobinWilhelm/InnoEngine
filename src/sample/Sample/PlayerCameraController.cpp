#include "PlayerCameraController.h"

#include <algorithm>

auto PlayerCameraController::create( InnoEngine::Ref<InnoEngine::Camera> camera, InnoEngine::Viewport view_port ) -> InnoEngine::Ref<PlayerCameraController>
{
    return InnoEngine::Ref<PlayerCameraController>( new PlayerCameraController( camera, view_port ) );
}

bool PlayerCameraController::handle_event( const SDL_Event& event )
{
    if ( is_mouse_in_viewport() == false )
        return false;

    switch ( event.type ) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        if ( event.key.scancode == SDL_SCANCODE_W ) {
            m_KeydownW = event.key.down;
            return true;
        }

        if ( event.key.scancode == SDL_SCANCODE_A ) {
            m_KeydownA = event.key.down;
            return true;
        }

        if ( event.key.scancode == SDL_SCANCODE_S ) {
            m_KeydownS = event.key.down;
            return true;
        }

        if ( event.key.scancode == SDL_SCANCODE_D ) {
            m_KeydownD = event.key.down;
            return true;
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        if ( m_LeftMouseButtonDown ) {
            m_MouseMove += { event.motion.xrel, event.motion.yrel };
        }
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        if ( event.button.button == SDL_BUTTON_LEFT ) {
            m_LeftMouseButtonDown = event.button.down;
            return true;
        }
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
    {
        m_MouseScroll.x += event.wheel.x;
        m_MouseScroll.y += event.wheel.y;
        return true;
    }
    }

    return false;
}

void PlayerCameraController::update( double delta_time )
{
    DXSM::Vector3 camera_pos = m_Camera->get_position();

    if ( m_MouseScroll.y != 0 ) {
        camera_pos.z  = std::clamp( camera_pos.z - static_cast<float>( camera_pos.z * m_ZoomSpeed * delta_time * m_MouseScroll.y ), 0.01f, 10.0f );
        m_MouseScroll = {};
    }

    float left_border  = m_BorderLeft + camera_pos.z * m_Viewport.Width / 2;
    float right_border = m_BorderRight - left_border;

    float bottom_border = m_BorderBottom + camera_pos.z * m_Viewport.Height / 2;
    float top_border    = m_BorderTop - bottom_border;

    if ( m_KeydownW && camera_pos.y < top_border )
        camera_pos.y += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

    if ( m_KeydownA && camera_pos.x > left_border )
        camera_pos.x -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

    if ( m_KeydownS && camera_pos.y > bottom_border )
        camera_pos.y -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

    if ( m_KeydownD && camera_pos.x < right_border )
        camera_pos.x += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

    if ( m_LeftMouseButtonDown ) {
        camera_pos.x -= m_MouseMove.x * camera_pos.z;
        camera_pos.y += m_MouseMove.y * camera_pos.z;
        m_MouseMove = {};
    }

    /*
    if ( left_border >= right_border )
        camera_pos.x = ( m_BorderRight + m_BorderLeft ) / 2;
    else
        camera_pos.x = std::clamp( camera_pos.x, left_border, right_border );

    if ( bottom_border >= top_border )
        camera_pos.y = ( m_BorderTop + m_BorderBottom ) / 2;
    else
        camera_pos.y = std::clamp( camera_pos.y, bottom_border, top_border );
    */
    m_Camera->set_position( camera_pos );
}

void PlayerCameraController::set_camera_borders( float left, float right, float top, float bottom )
{
    m_BorderLeft   = left;
    m_BorderRight  = right;
    m_BorderTop    = top;
    m_BorderBottom = bottom;
}
