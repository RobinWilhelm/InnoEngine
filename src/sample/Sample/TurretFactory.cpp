#include "TurretFactory.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/AssetManager.h"

#include "Turret.h"

InnoEngine::Ref<AAATurret> TurretFactory::create_mg_turret()
{
    InnoEngine::Ref<AAATurret> turret = InnoEngine::Ref<AAATurret>( new AAATurret );
    turret->m_WeaponScale             = { 0.1f, 0.1f };
    turret->m_WeaponRotationOrigin    = { 0.5f, 1.0f - 0.676f };
    turret->m_WeaponMuzzleOrigin      = { 0.727f, 1.0f };
    turret->m_ReloadTime              = 0.01f;
    turret->m_ElevationConstraints    = { -DirectX::XM_PI, DirectX::XM_PI };
    turret->m_ElevationOffset         = DirectX::XM_PIDIV2;
    turret->m_ElevationSpeed          = DirectX::XMConvertToRadians( 120.0f );
    turret->m_ProjectileMaxLifeTime   = 5.0f;
    turret->m_ProjectileSpeed         = 800.0f;
    turret->m_ProjectileDamage        = 0.02f;
    turret->m_Accuracy                = 80;

    if ( auto opt = InnoEngine::CoreAPI::get_assetmanager()->require_asset<InnoEngine::Texture2D>( "MG.png" ) ) {
        turret->m_Texture = opt.value().get();
    }

    if ( auto opt = InnoEngine::CoreAPI::get_assetmanager()->require_asset<InnoEngine::Texture2D>( "Bullet_MG.png" ) ) {
        turret->m_ProjectileTexture = opt.value().get();
    }

    DXSM::Vector2 tex_size      = { static_cast<float>( turret->m_Texture->get_specs().Width ), static_cast<float>( turret->m_Texture->get_specs().Height ) };
    DXSM::Vector2 muzzle_offset = turret->m_WeaponMuzzleOrigin - turret->m_WeaponRotationOrigin;
    muzzle_offset *= tex_size * turret->m_WeaponScale;
    turret->m_MuzzleRotationOriginDistance = muzzle_offset.Length();
    return turret;
}
