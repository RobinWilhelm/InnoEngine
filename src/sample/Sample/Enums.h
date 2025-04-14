#pragma once

enum class CollisionCategory : uint32_t
{
    Static     = 0x00000001,
    Dynamic    = 0x00000002,
    Projectile = 0x00000004,
};

enum class ShapeCategory : uint32_t
{
    Ground = 0,
    Building,
    Asteroid,
    Projectile,
};
