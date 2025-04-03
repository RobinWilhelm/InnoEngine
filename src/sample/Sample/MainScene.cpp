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

    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity    = { 0.0f, -10.0f };
    m_PhysicsWorldId     = b2CreateWorld( &world_def );

    b2BodyDef groundbody_def = b2DefaultBodyDef();
    groundbody_def.position  = { static_cast<float>( m_SceneWidth ) / 2, 25.0f };
    m_PhysicsGroundId        = b2CreateBody( m_PhysicsWorldId, &groundbody_def );
    b2Polygon ground_box     = b2MakeBox( static_cast<float>( m_SceneWidth ) / 2, 23.0f );

    b2ShapeDef groundshape_def = b2DefaultShapeDef();
    b2CreatePolygonShape( m_PhysicsGroundId, &groundshape_def, &ground_box );
}

GameScene::~GameScene()
{
    b2DestroyWorld( m_PhysicsWorldId );
}

void GameScene::update( double delta_time )
{
    m_LastAsteroidSpawn += static_cast<float>( delta_time );
    if ( m_LastAsteroidSpawn >= m_AsteroidSpawnTime ) {
        // spawn asteroid

        auto& new_asteroid        = m_Asteroids.emplace_back();
        new_asteroid.Size         = 5 + SDL_randf() * 20;
        new_asteroid.PositionNext = { SDL_randf() * m_SceneWidth, static_cast<float>( m_SceneHeight ) };
        new_asteroid.Position     = new_asteroid.PositionNext;

        b2BodyDef body_def         = b2DefaultBodyDef();
        body_def.type              = b2_dynamicBody;
        body_def.position          = { new_asteroid.Position.x, new_asteroid.Position.y };
        body_def.linearVelocity    = { 0, ( 20 + SDL_randf() * 200 ) * -1 };
        new_asteroid.PhysicsBodyId = b2CreateBody( m_PhysicsWorldId, &body_def );

        b2Circle circle = {};
        circle.center   = { 0.0f, 0.0f };
        circle.radius   = new_asteroid.Size;

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density    = 1.0f;
        shape_def.friction   = 0.3f;

        b2CreateCircleShape( new_asteroid.PhysicsBodyId, &shape_def, &circle );
        m_LastAsteroidSpawn = 0.0f;
    }

    b2World_Step( m_PhysicsWorldId, delta_time, 4 );

    auto asteroidIt = m_Asteroids.begin();
    while ( asteroidIt != m_Asteroids.end() ) {

        asteroidIt->Position     = asteroidIt->PositionNext;
        auto pos                 = b2Body_GetPosition( asteroidIt->PhysicsBodyId );
        asteroidIt->PositionNext = DXSM::Vector2( pos.x, pos.y );

        if ( asteroidIt->PositionNext.y - asteroidIt->Size + 2 <= 50 ) {
            b2DestroyBody( asteroidIt->PhysicsBodyId );
            asteroidIt = m_Asteroids.erase( asteroidIt );
            continue;
        }
        ++asteroidIt;
    }
}

void GameScene::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    const IE::RenderContext* rc_screen = renderer->acquire_rendercontext( m_Parent->get_fullscreen_rch() );
    if ( rc_screen == nullptr )
        return;

    rc_screen->add_quad( InnoEngine::Origin::BottomLeft, { 0.0f, 0.0f }, { static_cast<float>( m_SceneWidth ), static_cast<float>( 50 ) }, 0.0f, { 0.5f, 0.5f, 0.5f, 1.0f } );

    for ( auto& asteroid : m_Asteroids ) {
        float pos_x = std::lerp( asteroid.Position.x, asteroid.PositionNext.x, interp_factor );
        float pos_y = std::lerp( asteroid.Position.y, asteroid.PositionNext.y, interp_factor );

        rc_screen->add_circle( { pos_x, pos_y }, asteroid.Size, 0.0f, { 0.5f, 0.5f, 0.5f, 1.0f } );
    }
}

bool GameScene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}

void DefaultCannon::update( double delta_time )
{
}
