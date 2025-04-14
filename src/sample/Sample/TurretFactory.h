#pragma once
#include "InnoEngine/BaseTypes.h"

class World;
class AAATurret;

class TurretFactory
{
public:
    static InnoEngine::Ref<AAATurret> create_mg_turret( );
};
