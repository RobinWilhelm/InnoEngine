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
        DefaultCameraController( Ref<Camera> camera ) :
            CameraController( camera )
        { }

    public:
        [[nodiscard]]
        static auto create( Ref<Camera> camera ) -> Ref<DefaultCameraController>;

        void update( double delta_time ) override;

    private:
        float m_MovementSpeed = 800.0f;
        float m_ZoomSpeed = 200.0f;
    };
}    // namespace InnoEngine
