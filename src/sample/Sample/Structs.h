#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "box2d/box2d.h"

#include "Enums.h"

struct PhysicsBodyUserData
{
    ShapeCategory Catergory;
};

struct Projectile : PhysicsBodyUserData
{
    InnoEngine::Ref<InnoEngine::Texture2D> Texture;
    DXSM::Vector2                          Position;
    DXSM::Vector2                          PositionNext;
    DXSM::Vector2                          Velocity;
    b2BodyId                               PhysicsBodyId = {};
    b2ShapeId                              ShapeId       = {};
    float                                  LifeTime;
    float                                  DamageKinetic;
};

struct Asteroid : PhysicsBodyUserData
{
    float         Size;
    DXSM::Vector2 Position;
    DXSM::Vector2 PositionNext;
    DXSM::Vector2 Velocity;
    b2BodyId      PhysicsBodyId = {};
    float         Hitpoints;
};
