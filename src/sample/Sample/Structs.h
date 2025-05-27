#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "box2d/box2d.h"

#include "Enums.h"

#include <variant>

struct PhysicsBodyUserData
{
    ShapeCategory Catergory;
};

struct Projectile : PhysicsBodyUserData
{
    InnoEngine::Ref<InnoEngine::Texture2D> Texture;
    DXSM::Vector2                          Position;
    DXSM::Vector2                          PositionNext;
    b2BodyId                               PhysicsBodyId;
    b2ShapeId                              ShapeId;
    float                                  LifeTime;
    float                                  DamageKinetic;
    bool                                   Collided;

    Projectile()
    {
        Texture       = nullptr;
        Position      = {};
        PositionNext  = {};
        PhysicsBodyId = {};
        ShapeId       = {};
        LifeTime      = 0.0f;
        DamageKinetic = 0.0f;
        Collided      = false;
    };
};

struct Asteroid : PhysicsBodyUserData
{
    InnoEngine::Ref<InnoEngine::Texture2D> Texture;
    float                                  Size;
    float                                  Hitpoints;
    DXSM::Vector2                          Position;
    DXSM::Vector2                          PositionNext;
    b2BodyId                               PhysicsBodyId;

    Asteroid()
    {
        Texture       = nullptr;
        Size          = {};
        Position      = {};
        PositionNext  = {};
        PhysicsBodyId = {};
        Hitpoints     = 0.0f;
    };
};
