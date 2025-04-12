#include "World.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/InputSystem.h"

#include "TurretFactory.h"
#include "Turret.h"

World::~World()
{
    b2DestroyWorld( m_PhysicsWorldId );
}

InnoEngine::Ref<World> World::create( DXSM::Vector2 dimensions )
{
    InnoEngine::Ref<World> world = InnoEngine::Ref<World>( new World() );
    world->m_Dimensions          = dimensions;

    b2WorldDef world_def    = b2DefaultWorldDef();
    world_def.gravity       = { 0.0f, -10.0f };
    world->m_PhysicsWorldId = b2CreateWorld( &world_def );

    b2BodyDef groundbody_def = b2DefaultBodyDef();
    groundbody_def.position  = { static_cast<float>( dimensions.x ) / 2, 25.0f };
    world->m_PhysicsGroundId = b2CreateBody( world->m_PhysicsWorldId, &groundbody_def );
    b2Polygon ground_box     = b2MakeBox( static_cast<float>( dimensions.x ) / 2, 23.0f );

    b2ShapeDef groundshape_def = b2DefaultShapeDef();
    b2CreatePolygonShape( world->m_PhysicsGroundId, &groundshape_def, &ground_box );

    world->m_Asteroids.build_pool( 1000 );
    world->m_Projectiles.build_pool( 50000 );

    world->m_Turrets.push_back( TurretFactory::create_mg_turret( world.get(), { 100, 50 } ) );
    world->m_Turrets.push_back( TurretFactory::create_mg_turret( world.get(), { 200, 50 } ) );
    world->m_Turrets.push_back( TurretFactory::create_mg_turret( world.get(), { 300, 50 } ) );
    world->m_Turrets.push_back( TurretFactory::create_mg_turret( world.get(), { 400, 50 } ) );
    world->m_Turrets.push_back( TurretFactory::create_mg_turret( world.get(), { 500, 50 } ) );

    return world;
}

void World::update( double delta_time )
{

    m_LastAsteroidSpawn += static_cast<float>( delta_time );
    if ( m_LastAsteroidSpawn >= m_AsteroidSpawnTime ) {
        // spawn asteroid

        Asteroid* new_asteroid     = add_asteroid();
        new_asteroid->Size         = 5 + SDL_randf() * 20;
        new_asteroid->PositionNext = { SDL_randf() * m_Dimensions.x, static_cast<float>( m_Dimensions.y ) };
        new_asteroid->Position     = new_asteroid->PositionNext;

        b2BodyDef body_def          = b2DefaultBodyDef();
        body_def.type               = b2_dynamicBody;
        body_def.position           = { new_asteroid->Position.x, new_asteroid->Position.y };
        body_def.linearVelocity     = { 0, ( 20 + SDL_randf() * 200 ) * -1 };
        new_asteroid->PhysicsBodyId = b2CreateBody( m_PhysicsWorldId, &body_def );

        b2Circle circle = {};
        circle.center   = { 0.0f, 0.0f };
        circle.radius   = new_asteroid->Size;

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density    = 1.0f;
        shape_def.friction   = 0.3f;

        b2CreateCircleShape( new_asteroid->PhysicsBodyId, &shape_def, &circle );
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

    auto projectileIt = m_Projectiles.begin();
    while ( projectileIt != m_Projectiles.end() ) {
        projectileIt->Position     = projectileIt->PositionNext;
        auto pos                   = b2Body_GetPosition( projectileIt->PhysicsBodyId );
        projectileIt->PositionNext = DXSM::Vector2( pos.x, pos.y );
        ++projectileIt;
    }
    InnoEngine::Application* app   = InnoEngine::CoreAPI::get_application();
    InnoEngine::InputSystem* input = InnoEngine::CoreAPI::get_inputsystem();

    for ( auto turret : m_Turrets ) {
        turret->set_target( app->get_mouse_scene_pos() );
        if ( input->get_key_state( SDL_SCANCODE_SPACE ).Down )
            turret->fire();

        turret->update( delta_time );
    }
}

void World::render( float interp_factor, const InnoEngine::RenderContext* render_ctx )
{
    for ( Asteroid& asteroid : m_Asteroids ) {
        float pos_x = std::lerp( asteroid.Position.x, asteroid.PositionNext.x, interp_factor );
        float pos_y = std::lerp( asteroid.Position.y, asteroid.PositionNext.y, interp_factor );

        render_ctx->add_circle( { pos_x, pos_y },
                                asteroid.Size,
                                { 0.5f, 0.5f, 0.5f, 1.0f } );
    }

    for ( auto& projectile : m_Projectiles ) {
        float pos_x = std::lerp( projectile.Position.x, projectile.PositionNext.x, interp_factor );
        float pos_y = std::lerp( projectile.Position.y, projectile.PositionNext.y, interp_factor );

        b2Rot rotation = b2Body_GetRotation( projectile.PhysicsBodyId );

        render_ctx->add_textured_quad( projectile.Texture,
                                       { 0.0f, 0.0f, 1.0f, 1.0f },
                                       { pos_x, pos_y },
                                       InnoEngine::Origin::Middle,
                                       { 0.1f, 0.1f },
                                       DirectX::XMConvertToDegrees( b2Rot_GetAngle( rotation ) ) );
    }

    for ( auto turret : m_Turrets ) {
        turret->render( render_ctx );
    }
}

Projectile* World::add_projectile()
{
    return static_cast<Projectile*>( m_Projectiles.allocate() );
}

Asteroid* World::add_asteroid()
{
    return static_cast<Asteroid*>( m_Asteroids.allocate() );
}

b2WorldId World::get_physics_world()
{
    return m_PhysicsWorldId;
}
