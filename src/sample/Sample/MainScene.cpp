#include "MainScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"
#include "InnoEngine/graphics/RenderContext.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/InputSystem.h"

#include "Sample.h"
#include "World.h"
#include "PlayerCameraController.h"

#include "Turret.h"

#include <chrono>
#include <thread>
#include <optional>

GameScene::GameScene( SampleProject* parent ) :
    IE::Layer( parent )
{
    m_GameWorld = World::create( { 3000.0f, 6000.0f } );

    m_CameraController = PlayerCameraController::create( m_Parent->get_default_camera(), m_Parent->get_fullscreen_viewport() );
    m_CameraController->set_camera_borders( 0.0f, static_cast<float>( 3000.0f ), static_cast<float>( 6000.0f ), 0.0f );
    m_Parent->register_cameracontroller( m_CameraController );
}

GameScene::~GameScene()
{
}

void GameScene::update( double delta_time )
{
    m_GameWorld->update( delta_time );
}

void GameScene::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    const IE::RenderContext* rc_screen = renderer->acquire_rendercontext( m_Parent->get_fullscreen_rch() );
    if ( rc_screen == nullptr )
        return;

    rc_screen->add_quad( { 0.0f, 0.0f },
                         InnoEngine::Origin::BottomLeft,
                         { static_cast<float>( 3000.0f ), static_cast<float>( 50 ) },
                         { 0.5f, 0.5f, 0.5f, 1.0f } );

    rc_screen->add_circle( { 0, 0 },
                           2,
                           { 0.5f, 1.0f, 0.5f, 1.0f } );

    m_GameWorld->render( interp_factor, rc_screen );
}

bool GameScene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
