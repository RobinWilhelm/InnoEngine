#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/utility/ObjectPool.h"

#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/math_functions.h"

struct Projectile
{
    InnoEngine::Ref<InnoEngine::Texture2D> Texture;
    DXSM::Vector2                          Position;
    DXSM::Vector2                          PositionNext;
    DXSM::Vector2                          Velocity;
    b2BodyId                               PhysicsBodyId = {};
};

struct Asteroid
{
    float         Size;
    DXSM::Vector2 Position;
    DXSM::Vector2 PositionNext;
    DXSM::Vector2 Velocity;
    b2BodyId      PhysicsBodyId = {};
};

class Turret;

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



    b2WorldId   get_physics_world();

private:
    DXSM::Vector2 m_Dimensions        = { 0.0f, 0.0f };
    float         m_LastAsteroidSpawn = 0.0f;
    const float   m_AsteroidSpawnTime = 1.0f;

    InnoEngine::ObjectPool<Asteroid, uint16_t, InnoEngine::ObjectPoolType::RestoreSequence>   m_Asteroids;
    InnoEngine::ObjectPool<Projectile, uint32_t, InnoEngine::ObjectPoolType::RestoreSequence> m_Projectiles;

    std::vector<InnoEngine::Ref<Turret>> m_Turrets;

    b2WorldId m_PhysicsWorldId  = {};
    b2BodyId  m_PhysicsGroundId = {};
};
