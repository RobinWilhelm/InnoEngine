#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/CameraController.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/InputSystem.h"

namespace InnoEngine
{
    void CameraController::set_viewport( const Viewport& view_port )
    {
        m_Viewport = view_port;
    }

    bool CameraController::is_mouse_in_viewport() const
    {
        const InputSystem* input = CoreAPI::get_inputsystem();
        DXSM::Vector2 mouse_pos = input->get_mouse_position();
        if (mouse_pos.x < m_Viewport.LeftOffset ||
             mouse_pos.y < m_Viewport.TopOffset ||
             mouse_pos.x >= m_Viewport.LeftOffset + m_Viewport.Width ||
             mouse_pos.y >= m_Viewport.TopOffset + m_Viewport.Height)
        {
            return false;
        }
        return true;
    }
}    // namespace InnoEngine
