#include "World.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/InputSystem.h"

#include "Ground.h"
#include "BuildingFactory.h"
#include "Building.h"
#include "TurretFactory.h"
#include "Turret.h"

World::~World()
{
    b2DestroyWorld( m_PhysicsWorldId );
}

InnoEngine::Ref<World> World::create( DXSM::Vector2 dimensions )
{
    InnoEngine::Ref<World> world = InnoEngine::Ref<World>( new World() );

    b2WorldDef world_def    = b2DefaultWorldDef();
    world_def.gravity       = { 0.0f, -30.0f };
    world->m_PhysicsWorldId = b2CreateWorld( &world_def );

    world->m_Ground     = Ground::create( world.get(), { 0.0f, 0.0f }, { dimensions.x, dimensions.y } );
    world->m_Dimensions = dimensions;

    world->m_Asteroids.build_pool( 1000 );
    world->m_Projectiles.build_pool( 50000 );

    auto t1 = BuildingFactory::create_basic_defense_tower( world.get(), { 1500.0f, 50 } );
    t1->insert_turret( TurretFactory::create_mg_turret() );
    world->m_Buildings.push_back( t1 );

    return world;
}

void World::update( double delta_time )
{

    m_LastAsteroidSpawn += static_cast<float>( delta_time );
    if ( m_LastAsteroidSpawn >= m_AsteroidSpawnTime ) {
        // spawn asteroid

        Asteroid* new_asteroid     = add_asteroid();
        new_asteroid->Catergory    = ShapeCategory::Asteroid;
        new_asteroid->Size         = 5 + SDL_randf() * 20;
        new_asteroid->PositionNext = { SDL_randf() * m_Dimensions.x, static_cast<float>( m_Dimensions.y ) };
        new_asteroid->Position     = new_asteroid->PositionNext;
        new_asteroid->Hitpoints    = std::powf( new_asteroid->Size, 2.0f );

        b2BodyDef body_def          = b2DefaultBodyDef();
        body_def.type               = b2_dynamicBody;
        body_def.position           = { new_asteroid->Position.x, new_asteroid->Position.y };
        body_def.linearVelocity     = { 0, ( 20 + SDL_randf() * 200 ) * -1 };
        new_asteroid->PhysicsBodyId = b2CreateBody( m_PhysicsWorldId, &body_def );

        b2Circle circle = {};
        circle.center   = { 0.0f, 0.0f };
        circle.radius   = new_asteroid->Size;

        b2ShapeDef shape_def      = b2DefaultShapeDef();
        shape_def.density         = 1.0f;
        shape_def.friction        = 0.3f;
        shape_def.enableHitEvents = true;
        shape_def.userData        = static_cast<void*>( new_asteroid );

        b2CreateCircleShape( new_asteroid->PhysicsBodyId, &shape_def, &circle );
        m_LastAsteroidSpawn = 0.0f;
    }

    b2World_Step( m_PhysicsWorldId, delta_time, 4 );

    std::vector<b2ShapeId> collisions;
    b2ContactEvents        contactEvents = b2World_GetContactEvents( m_PhysicsWorldId );
    for ( int i = 0; i < contactEvents.hitCount; ++i ) {
        b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;
        //if ( hitEvent->approachSpeed > 10.0f ) {
            PhysicsBodyUserData* ud_a = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( hitEvent->shapeIdA ) );
            PhysicsBodyUserData* ud_b = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( hitEvent->shapeIdB ) );

            if ( ud_a == nullptr || ud_b == nullptr ) {
                continue;
            }

            if ( ud_a->Catergory == ShapeCategory::Projectile && ud_b->Catergory == ShapeCategory::Asteroid ) {
                resolve_collision_asteroid_projectile( hitEvent, static_cast<Asteroid*>( ud_b ), static_cast<Projectile*>( ud_a ) );
            }
            if ( ud_a->Catergory == ShapeCategory::Asteroid && ud_b->Catergory == ShapeCategory::Projectile ) {
                resolve_collision_asteroid_projectile( hitEvent, static_cast<Asteroid*>( ud_a ), static_cast<Projectile*>( ud_b ) );
            }

            if ( ud_a->Catergory == ShapeCategory::Ground && ud_b->Catergory == ShapeCategory::Asteroid ) {
                resolve_collision_asteroid_ground( hitEvent, static_cast<Asteroid*>( ud_b ), static_cast<Ground*>( ud_a ) );
            }
            if ( ud_a->Catergory == ShapeCategory::Asteroid && ud_b->Catergory == ShapeCategory::Ground ) {
                resolve_collision_asteroid_ground( hitEvent, static_cast<Asteroid*>( ud_a ), static_cast<Ground*>( ud_b ) );
            }
        //}
    }

    auto asteroidIt = m_Asteroids.begin();
    while ( asteroidIt != m_Asteroids.end() ) {

        float hitpoints = asteroidIt->Hitpoints;
        if (hitpoints <= 0.0f ) {
            b2DestroyBody( asteroidIt->PhysicsBodyId );
            asteroidIt = m_Asteroids.erase( asteroidIt );
            continue;
        }

        asteroidIt->Position     = asteroidIt->PositionNext;
        auto pos                 = b2Body_GetPosition( asteroidIt->PhysicsBodyId );
        asteroidIt->PositionNext = DXSM::Vector2( pos.x, pos.y );

        ++asteroidIt;
    }

    auto projectileIt = m_Projectiles.begin();
    while ( projectileIt != m_Projectiles.end() ) {
        projectileIt->Position     = projectileIt->PositionNext;
        auto pos                   = b2Body_GetPosition( projectileIt->PhysicsBodyId );
        projectileIt->PositionNext = DXSM::Vector2( pos.x, pos.y );

        projectileIt->LifeTime -= static_cast<float>( delta_time );

        if ( projectileIt->LifeTime <= 0.0f ) {
            b2DestroyBody( projectileIt->PhysicsBodyId );
            projectileIt->Texture.reset();
            projectileIt = m_Projectiles.erase( projectileIt );
            continue;
        }
        ++projectileIt;
    }
    InnoEngine::Application* app   = InnoEngine::CoreAPI::get_application();
    InnoEngine::InputSystem* input = InnoEngine::CoreAPI::get_inputsystem();

    for ( auto& building : m_Buildings ) {
        building->update( delta_time );
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

    for ( auto& building : m_Buildings ) {
        building->render( render_ctx );
    }
}

Projectile* World::add_projectile()
{
    Projectile* p = static_cast<Projectile*>( m_Projectiles.allocate() );
    p->Catergory  = ShapeCategory::Projectile;
    return p;
}

Asteroid* World::add_asteroid()
{
    return static_cast<Asteroid*>( m_Asteroids.allocate() );
}

b2WorldId World::get_physics_world()
{
    return m_PhysicsWorldId;
}

void World::resolve_collision_asteroid_projectile( b2ContactHitEvent* hit_event, Asteroid* asteroid, Projectile* projectile )
{
    asteroid->Hitpoints -= projectile->DamageKinetic * hit_event->approachSpeed;
    projectile->LifeTime = 0.0f;
}

void World::resolve_collision_asteroid_ground( b2ContactHitEvent* hit_event, Asteroid* asteroid, Ground* ground )
{
    asteroid->Hitpoints = 0.0f;
}

void World::resolve_collision_asteroid_building( b2ContactHitEvent* hit_event, Asteroid* asteroid, Building* building )
{
    asteroid->Hitpoints = 0.0f;
}
