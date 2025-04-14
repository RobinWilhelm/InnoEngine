#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"

class AAATurret;
class World;

class Building
{
protected:
    friend class BuildingFactory;
    Building() = default;

public:
    virtual ~Building() = default;

    virtual void update( double delta_time );
    virtual void render( const InnoEngine::RenderContext* render_ctx );

    bool insert_turret( InnoEngine::Ref<AAATurret> turret );

private:
    void update_turretslots( double delta_time );
    void render_turretslots( const InnoEngine::RenderContext* render_ctx );

private:
    World*                                 m_World = nullptr;
    DXSM::Vector2                          m_Position;
    InnoEngine::Ref<InnoEngine::Texture2D> m_Texture;

    struct TurretSlot
    {
        DXSM::Vector2              Offset;
        InnoEngine::Ref<AAATurret> Turret;
    };

    std::vector<TurretSlot> m_TurretSlots;
};

class DefenseTower : public Building
{
public:
    friend class BuildingFactory;
    DefenseTower() = default;
};
