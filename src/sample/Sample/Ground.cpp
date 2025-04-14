#include "Ground.h"
#include "World.h"

auto Ground::create( World* world, DXSM::Vector2 position, DXSM::Vector2 size ) -> InnoEngine::Ref<Ground>
{
    InnoEngine::Ref<Ground> ground = InnoEngine::Ref<Ground>( new Ground() );
    ground->Catergory              = ShapeCategory::Ground;
    ground->m_Position             = position;
    ground->m_Size                 = size;

    b2BodyDef groundbody_def = b2DefaultBodyDef();
    groundbody_def.position  = { static_cast<float>( size.x ) / 2, 25.0f };
    ground->m_PhysicsBodyId  = b2CreateBody( world->get_physics_world(), &groundbody_def );
    b2Polygon ground_box     = b2MakeBox( static_cast<float>( size.x ) / 2, 23.0f );

    b2ShapeDef shape_def      = b2DefaultShapeDef();
    shape_def.enableHitEvents = true;
    shape_def.userData        = static_cast<void*>( ground.get() );

    b2CreatePolygonShape( ground->m_PhysicsBodyId, &shape_def, &ground_box );

    return ground;
}
