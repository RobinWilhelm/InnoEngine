#include "Building.h"
#include "Turret.h"

void Building::update( double delta_time )
{
    update_turretslots( delta_time );
}

void Building::render( const InnoEngine::RenderContext* render_ctx )
{
    render_turretslots( render_ctx );
}

bool Building::insert_turret( InnoEngine::Ref<AAATurret> turret )
{
    for ( auto& slot : m_TurretSlots ) {
        if ( slot.Turret == nullptr ) {
            turret->m_World    = m_World;
            turret->m_Position = m_Position + slot.Offset;
            slot.Turret        = turret;
            return true;
        }
    }
    return false;
}

void Building::update_turretslots( double delta_time )
{
    for ( auto& slot : m_TurretSlots ) {
        if ( slot.Turret != nullptr ) {
            slot.Turret->update( delta_time );
        }
    }
}

void Building::render_turretslots( const InnoEngine::RenderContext* render_ctx )
{
    for ( auto& slot : m_TurretSlots ) {
        if ( slot.Turret != nullptr ) {
            slot.Turret->render( render_ctx );
        }
    }
}
