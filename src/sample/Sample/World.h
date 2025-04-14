#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/utility/ObjectPool.h"

#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/math_functions.h"

#include "Enums.h"

#include "Structs.h"

class Ground;
class Building;
class AAATurret;

class World
{
    World() = default;

public:
    ~World();
    static InnoEngine::Ref<World> create( DXSM::Vector2 dimensions );

    void update( double delta_time );
    void render( float interp_factor, const InnoEngine::RenderContext* render_ctx );

    Projectile* add_projectile();
    Asteroid*   add_asteroid();

    b2WorldId get_physics_world();
    void      resolve_collision_asteroid_projectile( b2ContactHitEvent* hit_event, Asteroid* asteroid, Projectile* projectile );
    void      resolve_collision_asteroid_ground( b2ContactHitEvent* hit_event, Asteroid* asteriod, Ground* ground );
    void      resolve_collision_asteroid_building( b2ContactHitEvent* hit_event, Asteroid* asteroid, Building* building );

private:
    DXSM::Vector2                          m_Dimensions = { 0.0f, 0.0f };
    InnoEngine::Ref<Ground>                m_Ground;
    std::vector<InnoEngine::Ref<Building>> m_Buildings;

    float       m_LastAsteroidSpawn = 0.0f;
    const float m_AsteroidSpawnTime = 1.0f;

    InnoEngine::ObjectPool<Asteroid, uint16_t, InnoEngine::ObjectPoolType::RestoreSequence>   m_Asteroids;
    InnoEngine::ObjectPool<Projectile, uint32_t, InnoEngine::ObjectPoolType::RestoreSequence> m_Projectiles;

    b2WorldId m_PhysicsWorldId = {};
};
