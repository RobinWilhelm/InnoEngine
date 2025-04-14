#pragma once
#include "InnoEngine/BaseTypes.h"
#include "box2d/box2d.h"

#include "Structs.h"

class World;

class Ground : public PhysicsBodyUserData
{
public:
    static auto create( World* world, DXSM::Vector2 position, DXSM::Vector2 size ) -> InnoEngine::Ref<Ground>;

private:
    b2BodyId      m_PhysicsBodyId = {};
    DXSM::Vector2 m_Position;
    DXSM::Vector2 m_Size;
};
