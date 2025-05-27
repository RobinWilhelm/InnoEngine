#include "Ground.h"
#include "World.h"

#include "InnoEngine/graphics/RenderContext.h"

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

    b2ShapeDef shape_def          = b2DefaultShapeDef();
    shape_def.enableHitEvents     = true;
    shape_def.userData            = static_cast<void*>( ground.get() );
    shape_def.filter.categoryBits = static_cast<uint32_t>( CollisionCategory::Dynamic );
    shape_def.filter.maskBits     = static_cast<uint32_t>( CollisionCategory::Static ) |
                                static_cast<uint32_t>( CollisionCategory::Dynamic ) |
                                static_cast<uint32_t>( CollisionCategory::Projectile );

    b2CreatePolygonShape( ground->m_PhysicsBodyId, &shape_def, &ground_box );

    return ground;
}

void Ground::render( const InnoEngine::RenderContext* render_ctx ) const
{
    render_ctx->add_quad( m_Position,
                          InnoEngine::Origin::BottomLeft,
                          m_Size,
                          { 0.5f, 0.5f, 0.5f, 1.0f } );
}
