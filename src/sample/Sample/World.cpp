#include "World.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/InputSystem.h"
#include "InnoEngine/IE_Assert.h"
#include "InnoEngine/AssetManager.h"

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

    world->m_Ground     = Ground::create( world.get(), { 0.0f, 0.0f }, { dimensions.x, 50.0f } );
    world->m_Dimensions = dimensions;

    world->m_Asteroids.build_pool( 1000 );
    world->m_Projectiles.build_pool( 50000 );

    auto t1 = BuildingFactory::create_basic_defense_tower( world.get(), { 1400.0f, 50 } );
    t1->insert_turret( TurretFactory::create_mg_turret() );

    auto t2 = BuildingFactory::create_basic_defense_tower(world.get(), {1500.0f, 50});
    t2->insert_turret(TurretFactory::create_mg_turret());

    auto t3 = BuildingFactory::create_basic_defense_tower(world.get(), {1600.0f, 50});
    t3->insert_turret(TurretFactory::create_mg_turret());

    world->m_Buildings.push_back( t1 );
    world->m_Buildings.push_back(t2);
    world->m_Buildings.push_back(t3);

    return world;
}

void World::update( double delta_time )
{

    m_LastAsteroidSpawn += static_cast<float>( delta_time );
    if ( m_LastAsteroidSpawn >= m_AsteroidSpawnTime ) {
        create_random_meteor();
        m_LastAsteroidSpawn = 0.0f;
    }

    b2World_Step( m_PhysicsWorldId, delta_time, 4 );

    /*
    b2ContactEvents contact_events = b2World_GetContactEvents( m_PhysicsWorldId );
    for ( int i = 0; i < contact_events.hitCount; ++i ) {
        b2ContactHitEvent*   hit_event = &contact_events.hitEvents[ i ];
        // if ( hitEvent->approachSpeed > 10.0f ) {
        PhysicsBodyUserData* ud_a      = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( hit_event->shapeIdA ) );
        PhysicsBodyUserData* ud_b      = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( hit_event->shapeIdB ) );

        if ( ud_a == nullptr || ud_b == nullptr ) {
            continue;
        }

        bool resolved = resolve_collision( hit_event, ud_a, ud_b );
        if ( resolved == false )
            resolved = resolve_collision( hit_event, ud_b, ud_a );
        IE_ASSERT( resolved );
        //}
    }
    */

    b2ContactEvents contact_events = b2World_GetContactEvents( m_PhysicsWorldId );
    for ( int i = 0; i < contact_events.beginCount; ++i ) {
        b2ContactBeginTouchEvent* begintouch_event = &contact_events.beginEvents[ i ];
        // if ( hitEvent->approachSpeed > 10.0f ) {
        PhysicsBodyUserData*      ud_a             = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( begintouch_event->shapeIdA ) );
        PhysicsBodyUserData*      ud_b             = static_cast<PhysicsBodyUserData*>( b2Shape_GetUserData( begintouch_event->shapeIdB ) );

        if ( ud_a == nullptr || ud_b == nullptr ) {
            continue;
        }

        b2ContactHitEvent hit_event;
        hit_event.approachSpeed = 90.0f;
        bool resolved           = resolve_collision( &hit_event, ud_a, ud_b );
        if ( resolved == false )
            resolved = resolve_collision( &hit_event, ud_b, ud_a );
        IE_ASSERT( resolved );
        //}
    }

    auto asteroidIt = m_Asteroids.begin();
    while ( asteroidIt != m_Asteroids.end() ) {
        IE_ASSERT( asteroidIt->Catergory == ShapeCategory::Asteroid );
        float hitpoints = asteroidIt->Hitpoints;
        if ( hitpoints <= 0.0f ) {
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
        IE_ASSERT( projectileIt->Catergory == ShapeCategory::Projectile );
        projectileIt->LifeTime -= static_cast<float>( delta_time );

        if ( projectileIt->LifeTime <= 0.0f ) {
            b2DestroyBody( projectileIt->PhysicsBodyId );
            projectileIt->Texture.reset();
            projectileIt = m_Projectiles.erase( projectileIt );
            continue;
        }

        projectileIt->Position     = projectileIt->PositionNext;
        auto pos                   = b2Body_GetPosition( projectileIt->PhysicsBodyId );
        projectileIt->PositionNext = DXSM::Vector2( pos.x, pos.y );

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
    m_Ground->render( render_ctx );

    for ( Asteroid& asteroid : m_Asteroids ) {
        float pos_x = std::lerp( asteroid.Position.x, asteroid.PositionNext.x, interp_factor );
        float pos_y = std::lerp( asteroid.Position.y, asteroid.PositionNext.y, interp_factor );

        b2Rot rotation = b2Body_GetRotation( asteroid.PhysicsBodyId );

        render_ctx->add_textured_quad( asteroid.Texture,
                                       { 0.0f, 0.0f, 1.0f, 1.0f },
                                       { pos_x, pos_y },
                                       InnoEngine::Origin::Middle,
                                       { 1.0f, 1.0f },
                                       DirectX::XMConvertToDegrees( b2Rot_GetAngle( rotation ) ) );
    }

    for ( auto& projectile : m_Projectiles ) {
        float pos_x = std::lerp( projectile.Position.x, projectile.PositionNext.x, interp_factor );
        float pos_y = std::lerp( projectile.Position.y, projectile.PositionNext.y, interp_factor );

        b2Rot rotation = b2Body_GetRotation( projectile.PhysicsBodyId );

        DXSM::Color color = projectile.Collided ? DXSM::Color( 0.0f, 1.0f, 0.0f, 1.0f ) : DXSM::Color( 1.0f, 1.0f, 1.0f, 1.0f );
        render_ctx->add_textured_quad( projectile.Texture,
                                       { 0.0f, 0.0f, 1.0f, 1.0f },
                                       { pos_x, pos_y },
                                       InnoEngine::Origin::Middle,
                                       { 0.1f, 0.1f },
                                       DirectX::XMConvertToDegrees( b2Rot_GetAngle( rotation ) ),
                                       { 0.5f, 0.5f },
                                       color );
    }

    for ( auto& building : m_Buildings ) {
        building->render( render_ctx );
    }
}

Projectile* World::add_projectile()
{
    Projectile* p = static_cast<Projectile*>( m_Projectiles.create() );
    p->Catergory  = ShapeCategory::Projectile;
    return p;
}

Asteroid* World::add_asteroid()
{
    return static_cast<Asteroid*>( m_Asteroids.create() );
}

void World::create_random_meteor()
{

    Asteroid* new_asteroid  = add_asteroid();
    new_asteroid->Catergory = ShapeCategory::Asteroid;

    auto     asset_manager = InnoEngine::CoreAPI::get_assetmanager();
    uint32_t meteor_size   = static_cast<uint32_t>( SDL_rand( 3 ) );
    switch ( meteor_size ) {
    case 0:
    {
        uint32_t meteor_type = static_cast<uint32_t>( SDL_rand( 3 ) + 1 );

        if ( meteor_type > 1 )
            meteor_type += 1;
        meteor_type *= 10000;
        meteor_type += static_cast<uint32_t>( SDL_rand( 16 ) );

        if ( auto opt = asset_manager->require_asset<InnoEngine::Texture2D>( std::format( "Meteors/large/c{}.png", meteor_type ), false ) ) {
            new_asteroid->Texture = opt.value().get();
        }
        new_asteroid->Size = 60;
        break;
    }
    case 1:
    {
        uint32_t meteor_type = static_cast<uint32_t>( SDL_rand( 3 ) + 1 );

        if ( meteor_type > 1 )
            meteor_type += 1;
        meteor_type *= 10000;
        meteor_type += static_cast<uint32_t>( SDL_rand( 16 ) );

        if ( auto opt = asset_manager->require_asset<InnoEngine::Texture2D>( std::format( "Meteors/medium/d{}.png", meteor_type ), false ) ) {
            new_asteroid->Texture = opt.value().get();
        }
        new_asteroid->Size = 30;
        break;
    }
    case 2:
    {
        uint32_t meteor_type = static_cast<uint32_t>( SDL_rand( 3 ) + 1 );

        if ( meteor_type > 1 )
            meteor_type += 1;
        meteor_type *= 10000;
        meteor_type += static_cast<uint32_t>( SDL_rand( 16 ) );

        if ( auto opt = asset_manager->require_asset<InnoEngine::Texture2D>( std::format( "Meteors/small/a{}.png", meteor_type ), false ) ) {
            new_asteroid->Texture = opt.value().get();
        }
        new_asteroid->Size = 15;
        break;
    }
    }

    new_asteroid->PositionNext = { 1500, static_cast<float>( m_Dimensions.y ) };
    new_asteroid->Position     = new_asteroid->PositionNext;
    new_asteroid->Hitpoints    = std::powf( new_asteroid->Size / 10, 4.0f );

    b2BodyDef body_def          = b2DefaultBodyDef();
    body_def.type               = b2_dynamicBody;
    body_def.position           = { new_asteroid->Position.x, new_asteroid->Position.y };
    body_def.linearVelocity     = { 0, ( 20 + SDL_randf() * 200 ) * -1 };
    new_asteroid->PhysicsBodyId = b2CreateBody( m_PhysicsWorldId, &body_def );

    b2Circle circle = {};
    circle.center   = { 0.0f, 0.0f };
    circle.radius   = new_asteroid->Size;

    b2ShapeDef shape_def          = b2DefaultShapeDef();
    shape_def.density             = 3.0f + SDL_randf() * 5.0f;
    shape_def.friction            = 0.3f;
    shape_def.enableHitEvents     = true;
    shape_def.userData            = static_cast<void*>( new_asteroid );
    shape_def.filter.categoryBits = static_cast<uint32_t>( CollisionCategory::Dynamic );
    shape_def.filter.maskBits     = static_cast<uint32_t>( CollisionCategory::Static ) |
                                static_cast<uint32_t>( CollisionCategory::Dynamic ) |
                                static_cast<uint32_t>( CollisionCategory::Projectile );

    b2CreateCircleShape( new_asteroid->PhysicsBodyId, &shape_def, &circle );
}

b2WorldId World::get_physics_world()
{
    return m_PhysicsWorldId;
}

bool World::resolve_collision( b2ContactHitEvent* hit_event, PhysicsBodyUserData* left, PhysicsBodyUserData* right )
{
    switch ( left->Catergory ) {
    case ShapeCategory::Ground:
        switch ( right->Catergory ) {
        case ShapeCategory::Ground:
            return true;
        case ShapeCategory::Building:
            return true;
        case ShapeCategory::Asteroid:
            resolve_collision_asteroid_ground( hit_event, static_cast<Asteroid*>( right ), static_cast<Ground*>( left ) );
            return true;
        case ShapeCategory::Projectile:
            return true;
        }
        break;
    case ShapeCategory::Building:
        switch ( right->Catergory ) {
        case ShapeCategory::Building:
            return true;
        case ShapeCategory::Asteroid:
            return true;
        case ShapeCategory::Projectile:
            return true;
        }
        break;
    case ShapeCategory::Asteroid:
        switch ( right->Catergory ) {
        case ShapeCategory::Asteroid:
            return true;
        case ShapeCategory::Projectile:
            resolve_collision_asteroid_projectile( hit_event, static_cast<Asteroid*>( left ), static_cast<Projectile*>( right ) );
            return true;
        }
        break;
    case ShapeCategory::Projectile:
        switch ( right->Catergory ) {
        case ShapeCategory::Projectile:
            IE_ASSERT( false );
            return true;
        }
        break;
    }
    return false;
}

void World::resolve_collision_asteroid_projectile( b2ContactHitEvent* hit_event, Asteroid* asteroid, Projectile* projectile )
{
    IE_ASSERT( asteroid->Catergory == ShapeCategory::Asteroid );
    IE_ASSERT( projectile->Catergory == ShapeCategory::Projectile );

    asteroid->Hitpoints -= projectile->DamageKinetic * hit_event->approachSpeed;
    projectile->Collided = true;
    // projectile->LifeTime = 0.0f;
}

void World::resolve_collision_asteroid_ground( b2ContactHitEvent* hit_event, Asteroid* asteroid, Ground* ground )
{
    IE_ASSERT( asteroid->Catergory == ShapeCategory::Asteroid );
    asteroid->Hitpoints = 0.0f;
}

void World::resolve_collision_asteroid_building( b2ContactHitEvent* hit_event, Asteroid* asteroid, Building* building )
{
    IE_ASSERT( asteroid->Catergory == ShapeCategory::Asteroid );
    asteroid->Hitpoints = 0.0f;
}
