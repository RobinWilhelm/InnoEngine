#include "Turret.h"
#include "World.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Renderer.h"

void Turret::set_target( DXSM::Vector2 target )
{
    if ( m_ManualTarget != target ) {
        m_ManualTarget = target;

        float cos_elevation = cosf( m_CurrentElevation );
        float sin_elevation = sinf( m_CurrentElevation );

        DXSM::Vector2 tex_size = { static_cast<float>( m_Texture->get_specs().Width ), static_cast<float>( m_Texture->get_specs().Height ) };
        tex_size *= m_WeaponScale;

        DXSM::Vector2 muzzle_offset = m_WeaponMuzzleOrigin - m_WeaponRotationOrigin;
        muzzle_offset *= tex_size;
        DXSM::Vector2 rotated_muzzle_offset = { cos_elevation * muzzle_offset.x + -sin_elevation * muzzle_offset.y,
                                                sin_elevation * muzzle_offset.x + cos_elevation * muzzle_offset.y };

        DXSM::Vector2 to_target_from_origin = m_ManualTarget - m_RotationOriginPosition;
        float         distance              = to_target_from_origin.Length();

        if ( distance <= m_MuzzleRotationOriginDistance * 3.0f )
            return;

        float         rotation_origin_x                = m_RotationOriginPosition.x;
        DXSM::Vector2 to_target_from_origin_normalized = m_ManualTarget - m_RotationOriginPosition - rotated_muzzle_offset;
        to_target_from_origin_normalized.Normalize();

        m_OffsetTarget                                = m_RotationOriginPosition + to_target_from_origin_normalized * distance;
        DXSM::Vector2 to_simulated_target_from_origin = m_OffsetTarget - m_RotationOriginPosition;
        if ( to_simulated_target_from_origin.y > 0.0f ) {
            m_TargetElevation = asin( -to_simulated_target_from_origin.x / to_simulated_target_from_origin.Length() );
        }
        else {
            if ( to_simulated_target_from_origin.x > 0.0f ) {
                m_TargetElevation = -DirectX::XM_PIDIV2 - ( DirectX::XM_PIDIV2 + asin( -to_simulated_target_from_origin.x / to_simulated_target_from_origin.Length() ) );
            }
            else {
                m_TargetElevation = DirectX::XM_PIDIV2 + ( DirectX::XM_PIDIV2 - asin( -to_simulated_target_from_origin.x / to_simulated_target_from_origin.Length() ) );
            }
        }
    }
}

void Turret::update( double delta_time )
{
    if ( m_CurrentElevation != m_TargetElevation ) {

        if ( m_CurrentElevation > m_TargetElevation ) {
            m_CurrentElevation = max( m_TargetElevation, m_CurrentElevation - m_ElevationSpeed * delta_time );
        }
        else if ( m_CurrentElevation < m_TargetElevation ) {
            m_CurrentElevation = min( m_TargetElevation, m_CurrentElevation + m_ElevationSpeed * delta_time );
        }

        m_CurrentElevation = std::clamp( m_CurrentElevation, m_ElevationConstraints.x, m_ElevationConstraints.y );

        float         cos_elevation = cosf( m_CurrentElevation );
        float         sin_elevation = sinf( m_CurrentElevation );
        DXSM::Vector2 tex_size      = { static_cast<float>( m_Texture->get_specs().Width ), static_cast<float>( m_Texture->get_specs().Height ) };
        tex_size *= m_WeaponScale;

        DXSM::Vector2 muzzle_offset = m_WeaponMuzzleOrigin - m_WeaponRotationOrigin;
        muzzle_offset *= tex_size;

        DXSM::Vector2 rotated_muzzle_offset = { cos_elevation * muzzle_offset.x + -sin_elevation * muzzle_offset.y,
                                                sin_elevation * muzzle_offset.x + cos_elevation * muzzle_offset.y };

        m_WeaponMuzzlePosition = rotated_muzzle_offset + m_RotationOriginPosition;

        // why do i an offset here???
        m_WeaponMuzzleDirection = { cosf( m_CurrentElevation + m_ElevationOffset ), sinf( m_CurrentElevation + m_ElevationOffset ) };
        m_WeaponMuzzleDirection.Normalize();
    }

    if ( m_ReloadProgress <= 1.0f )
        m_ReloadProgress += 1.0f / m_ReloadTime * delta_time;
}

void Turret::render( const InnoEngine::RenderContext* render_ctx )
{
    render_ctx->add_quad( m_Position,
                          InnoEngine::Origin::Middle,
                          { 10.0f, 20.0f },
                          { 0.0f, 1.0f, 0.0f, 1.0f } );

    render_ctx->add_textured_quad( m_Texture,
                                   { 0.0f, 0.0f, 1.0f, 1.0f },
                                   m_Position + m_WeaponOffset,
                                   InnoEngine::Origin::RotationOrigin,
                                   m_WeaponScale,
                                   DirectX::XMConvertToDegrees( m_CurrentElevation ),
                                   m_WeaponRotationOrigin );

    /*
    render_ctx->add_line( m_WeaponMuzzlePosition, m_WeaponMuzzlePosition + m_WeaponMuzzleDirection * 10.0f, { 1.0f, 0.0f, 1.0f, 1.0f }, 1.0f, 0.5f );

    render_ctx->add_line( m_Position + m_WeaponOffset, m_OffsetTarget, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, 0.5f );

    // render_ctx->add_circle( m_Position + m_WeaponOffset, 1, { 1.0f, 0.0f, 1.0f, 1.0f } );
    // render_ctx->add_circle( m_OffsetPoint, 1, { 1.0f, 0.0f, 1.0f, 1.0f } );

    render_ctx->add_text( InnoEngine::CoreAPI::get_gpurenderer()->get_debug_font(),
                          m_Position + DXSM::Vector2 { 10, 100 },
                          15,
                          std::format( "{}", m_CurrentElevation ),
                          { 1.0f, 1.0f, 1.0f, 1.0f } );
    */
}

void Turret::fire()
{
    if ( m_ReloadProgress >= 1.0f ) {
        Projectile* new_projectile   = m_World->add_projectile();
        new_projectile->PositionNext = m_WeaponMuzzlePosition;
        new_projectile->Position     = m_WeaponMuzzlePosition;
        new_projectile->Texture      = m_ProjectileTexture;

        b2BodyDef body_def            = b2DefaultBodyDef();
        body_def.type                 = b2_dynamicBody;
        body_def.position             = { new_projectile->Position.x, new_projectile->Position.y };
        body_def.linearVelocity       = { m_WeaponMuzzleDirection.x * 900 + SDL_randf() * ( 100 - m_Accuracy ), m_WeaponMuzzleDirection.y * 900 + SDL_randf() * ( 100 - m_Accuracy ) };
        body_def.isBullet             = true;
        body_def.rotation             = b2MakeRot( m_CurrentElevation );
        new_projectile->PhysicsBodyId = b2CreateBody( m_World->get_physics_world(), &body_def );

        b2Circle circle = {};
        circle.center   = { 0.0f, 0.0f };
        circle.radius   = 1;

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density    = 1.0f;
        shape_def.friction   = 0.3f;

        b2CreateCircleShape( new_projectile->PhysicsBodyId, &shape_def, &circle );

        m_ReloadProgress = 0.0f;
    }
}
