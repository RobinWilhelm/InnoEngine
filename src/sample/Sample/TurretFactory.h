#pragma once
#include "InnoEngine/BaseTypes.h"

class World;
class Turret;

class TurretFactory
{
public:
    static InnoEngine::Ref<Turret> create_mg_turret(World* world, DXSM::Vector2 position );
};
