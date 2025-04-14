#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/RenderContext.h"
#include "InnoEngine/graphics/Texture2D.h"

class World;

class AAATurret
{
    friend class TurretFactory;
    friend class Building;
    AAATurret() = default;

public:
    void set_target( DXSM::Vector2 target );
    void update( double delta_time );
    void render( const InnoEngine::RenderContext* render_ctx );

    void fire();

protected:
    World*    m_World  = nullptr;
    Building* m_Parent = nullptr;

    DXSM::Vector2 m_Position;

    float         m_ElevationOffset      = 0.0f;
    DXSM::Vector2 m_ElevationConstraints = { 0.0f, 180.0f };
    float         m_ElevationSpeed       = 0.0f;
    float         m_CurrentElevation     = 0.0f;
    float         m_TargetElevation      = 0.0f;

    DXSM::Vector2 m_WeaponScale          = { 1.0f, 1.0f };
    DXSM::Vector2 m_WeaponRotationOrigin = { 0.5f, 0.5f };
    DXSM::Vector2 m_WeaponMuzzleOrigin   = { 0.5f, 0.5f };

    DXSM::Vector2 m_WeaponMuzzlePosition         = { 0.0f, 0.0f };
    DXSM::Vector2 m_WeaponMuzzleDirection        = { 0.0f, 0.0f };
    float         m_MuzzleRotationOriginDistance = 0.0f;

    DXSM::Vector2 m_OffsetPoint  = { 0.0f, 0.0f };
    DXSM::Vector2 m_OffsetTarget = { 0.0f, 0.0f };

    float m_Accuracy = 100.0f;

    float m_ReloadTime       = 1.0f;
    float m_ReloadProgress   = 1.0f;
    float m_ProjectileSpeed  = 900.0f;
    float m_ProjectileDamage = 10.0f;

    float m_ProjectileMaxLifeTime = 1.0f;

    DXSM::Vector2 m_ManualTarget = {};

    InnoEngine::Ref<InnoEngine::Texture2D> m_Texture;
    InnoEngine::Ref<InnoEngine::Texture2D> m_ProjectileTexture;
};
