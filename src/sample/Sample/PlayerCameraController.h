
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Camera.h"
#include "InnoEngine/graphics/CameraController.h"

class PlayerCameraController : public InnoEngine::CameraController
{
    PlayerCameraController( InnoEngine::Ref<InnoEngine::Camera> camera, InnoEngine::Viewport view_port ) :
        InnoEngine::CameraController( camera, view_port )
    { }

public:
    static auto create( InnoEngine::Ref<InnoEngine::Camera> camera, InnoEngine::Viewport view_port ) -> InnoEngine::Ref<PlayerCameraController>;

    // Inherited via CameraController
    bool handle_event( const SDL_Event& event ) override;
    void update( double delta_time ) override;

    void set_camera_borders( float left, float right, float top, float bottom );

private:
    bool m_KeydownW = false;
    bool m_KeydownA = false;
    bool m_KeydownS = false;
    bool m_KeydownD = false;

    bool          m_LeftMouseButtonDown = false;
    DXSM::Vector2 m_MouseMove           = {};
    DXSM::Vector2 m_MouseScroll         = {};

    float m_MovementSpeed = 800.0f;
    float m_ZoomSpeed     = 1.0f;

    float m_BorderLeft   = 0.0f;
    float m_BorderRight  = 0.0f;
    float m_BorderTop    = 0.0f;
    float m_BorderBottom = 0.0f;
};
