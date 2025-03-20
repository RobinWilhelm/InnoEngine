#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/DefaultCameraController.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/InputSystem.h"
#include "SDL3/SDL_events.h"

namespace InnoEngine
{
    auto DefaultCameraController::create( Ref<Camera> camera ) -> Ref<DefaultCameraController>
    {
        return Ref<DefaultCameraController>( new DefaultCameraController( camera ) );
    }

    void DefaultCameraController::update( double delta_time )
    {
        InputSystem*  input      = CoreAPI::get_inputsystem();
        DXSM::Vector3 camera_pos = m_Camera->get_position();

        if ( input->is_key_down( SDL_SCANCODE_W ) ) {
            camera_pos.y -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );
        }

        if ( input->is_key_down( SDL_SCANCODE_A ) ) {
            camera_pos.x -= static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );
        }

        if ( input->is_key_down( SDL_SCANCODE_S ) ) {
            camera_pos.y += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );
        }

        if ( input->is_key_down( SDL_SCANCODE_D ) ) {
            camera_pos.x += static_cast<float>( m_MovementSpeed * delta_time * camera_pos.z );
        }

        const auto& scroll_data = input->get_mouse_wheel_scroll();
        if ( scroll_data.y != 0 ) {
            camera_pos.z = std::clamp( camera_pos.z - static_cast<float>( m_ZoomSpeed * delta_time * scroll_data.y ), 0.01f, 10.0f );
        }

        m_Camera->set_position( camera_pos );
    }
}    // namespace InnoEngine
