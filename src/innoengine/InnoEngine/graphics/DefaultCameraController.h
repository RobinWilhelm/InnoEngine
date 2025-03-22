#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Camera.h"

#include "InnoEngine/graphics/CameraController.h"

namespace InnoEngine
{
    class DefaultCameraController :
        public CameraController
    {
    private:
        DefaultCameraController( Ref<Camera> camera, Viewport view_port) :
            CameraController( camera, view_port )
        { }

    public:
        [[nodiscard]]
        static auto create( Ref<Camera> camera, Viewport view_port ) -> Ref<DefaultCameraController>;

        virtual bool handle_event( const SDL_Event& event ) override;
        virtual void update( double delta_time ) override;

    private:
        bool m_KeydownW = false;
        bool m_KeydownA = false;
        bool m_KeydownS = false;
        bool m_KeydownD = false;

        bool          m_LeftMouseButtonDown = false;
        DXSM::Vector2 m_MouseMove           = {};
        DXSM::Vector2 m_MouseScroll         = {};

        float m_MovementSpeed = 80.0f;
        float m_ZoomSpeed     = 10.0f;
    };
}    // namespace InnoEngine
