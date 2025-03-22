#pragma once
#include "SDL3/SDL_events.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Viewport.h"

namespace InnoEngine
{
    class Camera;

    class CameraController
    {
    protected:
        CameraController( Ref<Camera> camera, Viewport view_port ) :
            m_Camera( camera ), m_Viewport( view_port ) { };

    public:
        virtual ~CameraController() = default;

        virtual bool handle_event( const SDL_Event& event ) = 0;
        virtual void update( double delta_time )            = 0;

        void set_viewport( const Viewport& view_port );

        bool is_mouse_in_viewport() const;

    protected:
        Ref<Camera> m_Camera;
        Viewport    m_Viewport;
    };
}    // namespace InnoEngine
