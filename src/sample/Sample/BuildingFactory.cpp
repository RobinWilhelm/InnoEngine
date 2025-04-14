#include "BuildingFactory.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/AssetManager.h"

#include "Building.h"

InnoEngine::Ref<DefenseTower> BuildingFactory::create_basic_defense_tower( World* world, DXSM::Vector2 position )
{
    InnoEngine::Ref<DefenseTower> building = InnoEngine::Ref<DefenseTower>( new DefenseTower() );
    building->m_World                      = world;
    building->m_Position                   = position;
    building->m_TurretSlots.push_back( {
        { 0.0f, 10.0f },
        nullptr
    } );

    if ( auto opt = InnoEngine::CoreAPI::get_assetmanager()->require_asset<InnoEngine::Texture2D>( "Bullet_MG.png" ) ) {

        building->m_Texture = opt.value().get();
    }
    return building;
}
