#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/DefaultCameraController.h"

#include "SDL3/SDL_events.h"

namespace InnoEngine
{
    auto DefaultCameraController::create( Ref<Camera> camera, Viewport view_port ) -> Ref<DefaultCameraController>
    {
        return Ref<DefaultCameraController>( new DefaultCameraController( camera, view_port ) );
    }

    bool DefaultCameraController::handle_event( const SDL_Event& event )
    {
        if(is_mouse_in_viewport() == false)
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
            m_MouseMove = { event.motion.xrel, event.motion.yrel };
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
            m_MouseScroll.x = event.wheel.x;
            m_MouseScroll.y = event.wheel.y;
            return true;
        }
        }

        return false;
    }

    void DefaultCameraController::update( double delta_time )
    {
        DXSM::Vector3 camera_pos = m_Camera->get_position();

        if ( m_KeydownW )
            camera_pos.y -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

        if ( m_KeydownA )
            camera_pos.x -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

        if ( m_KeydownS )
            camera_pos.y += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

        if ( m_KeydownD )
            camera_pos.x += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );

        if ( m_MouseScroll.y != 0 ) {
            camera_pos.z    = std::clamp( camera_pos.z - static_cast<float>( camera_pos.z * m_ZoomSpeed * delta_time * m_MouseScroll.y ), 0.01f, 10.0f );
            m_MouseScroll.y = 0;
        }

        if ( m_LeftMouseButtonDown ) {
            camera_pos.x -= m_MouseMove.x * camera_pos.z;
            camera_pos.y -= m_MouseMove.y * camera_pos.z;
            m_MouseMove = {};
        }
        m_Camera->set_position( camera_pos );
    }
}    // namespace InnoEngine
