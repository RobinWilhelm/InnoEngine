#pragma once
#include "InnoEngine/BaseTypes.h"

namespace InnoEngine
{
    class Camera;

    class CameraController
    {
    protected:
        CameraController( Ref<Camera> camera ) :
            m_Camera( camera ) { };

    public:
        virtual ~CameraController()              = default;
        virtual void update( double delta_time ) = 0;

    protected:
        Ref<Camera> m_Camera;
    };
}    // namespace InnoEngine
