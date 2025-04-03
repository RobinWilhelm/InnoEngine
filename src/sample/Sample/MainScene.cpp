#include "MainScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"
#include "InnoEngine/graphics/RenderContext.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Window.h"

#include "Sample.h"
#include "PlayerCameraController.h"

#include <chrono>
#include <thread>
#include <optional>

GameScene::GameScene( SampleProject* parent ) :
    IE::Layer( parent )
{
    m_CameraController = PlayerCameraController::create( m_Parent->get_default_camera(), m_Parent->get_fullscreen_viewport() );
    m_CameraController->set_camera_borders( 0.0f, static_cast<float>( m_SceneWidth ), static_cast<float>( m_SceneHeight ), 0.0f );
    m_Parent->register_cameracontroller( m_CameraController );
}

void GameScene::update( double delta_time )
{
}

void GameScene::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    const IE::RenderContext* rc_screen = renderer->acquire_rendercontext( m_Parent->get_fullscreen_rch() );
    if ( rc_screen == nullptr )
        return;

    rc_screen->add_quad( InnoEngine::Origin::BottomLeft, { 0.0f, 0.0f }, { static_cast<float>( m_SceneWidth ), static_cast<float>( 50 ) }, 0.0f, { 0.5f, 0.5f, 0.5f, 1.0f } );

    rc_screen->add_circle( { 1000, 1000 }, 100, 0.1f, { 0.5f, 0.5f, 0.5f, 1.0f } );
    rc_screen->add_circle( { 100, 100 }, 10, 0.1f, { 0.5f, 0.5f, 0.5f, 1.0f } );
    rc_screen->add_circle( { 2000, 5500 }, 100, 0.2f, { 1.0f, 0.5f, 0.0f, 1.0f } );
    rc_screen->add_circle( { 2500, 100 }, 10, 0.1f, { 0.5f, 0.5f, 0.5f, 1.0f } );


    rc_screen->add_line( {0.0f, 0.0f}, {1920, 1080}, 3.0f, 0.1f, {0.5f, 0.9f, 0.5f, 1.0f});
}

bool GameScene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
