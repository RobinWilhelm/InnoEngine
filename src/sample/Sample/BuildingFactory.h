#pragma once
#include "InnoEngine/BaseTypes.h"

class World;
class DefenseTower;

class BuildingFactory
{
public:
    static auto create_basic_defense_tower( World* world, DXSM::Vector2 position) -> InnoEngine::Ref<DefenseTower>;
};
