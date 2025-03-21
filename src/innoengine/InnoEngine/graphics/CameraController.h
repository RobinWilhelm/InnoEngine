#pragma once
#include "InnoEngine/BaseTypes.h"
#include "SDL3/SDL_events.h"

namespace InnoEngine
{
    class Camera;

    class CameraController
    {
    protected:
        CameraController( Ref<Camera> camera ) :
            m_Camera( camera ) { };

    public:
        virtual ~CameraController() = default;

        virtual bool handle_event( const SDL_Event& event ) = 0;
        virtual void update( double delta_time) = 0;

    protected:
        Ref<Camera> m_Camera;
    };
}    // namespace InnoEngine
