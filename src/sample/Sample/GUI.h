#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/RenderContext.h"
#include "InnoEngine/Layer.h"
#include "InnoEngine/graphics/Camera.h"

namespace IE = InnoEngine;

class SampleProject;
class World;

class GUI : public IE::Layer
{
    GUI( SampleProject* parent, IE::Ref<World> game_world );

public:
    static auto create( SampleProject* parent, IE::Ref<World> game_world ) -> IE::Own<GUI>;
    ~GUI();

    // Inherited via Layer
    void update( double delta_time ) override;
    void render( float interp_factor, IE::GPURenderer* renderer ) override;
    bool handle_event( const SDL_Event& event ) override;

private:
    IE::Ref<World>          m_GameWorld;
    IE::Ref<IE::Camera>     m_UICamera;
    IE::RenderContextHandle m_UIRenderContextHandle;

    DXSM::Vector2 m_ScreenDimensions;

    IE::Ref<IE::Texture2D> m_BuildingMenuIcon;
};
