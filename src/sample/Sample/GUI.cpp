#include "GUI.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/RenderContext.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/OrthographicCamera.h"

#include "Sample.h"
#include "World.h"

GUI::GUI( SampleProject* parent, IE::Ref<World> game_world ) :
    IE::Layer( parent )
{
    IE_ASSERT( parent != nullptr );
    IE_ASSERT( game_world != nullptr );

    m_GameWorld = game_world;

    auto fullscreen_viewport = parent->get_fullscreen_viewport();
    m_ScreenDimensions       = DXSM::Vector2( fullscreen_viewport.Width, fullscreen_viewport.Height );

    m_UICamera = IE::OrthographicCamera::create( m_ScreenDimensions );

    IE::RenderContextSpecifications render_ctx_ui = {};
    render_ctx_ui.Camera                          = m_UICamera;
    render_ctx_ui.Viewport                        = fullscreen_viewport;
    m_UIRenderContextHandle                       = parent->get_renderer()->create_rendercontext( render_ctx_ui );

    auto asset_manager = IE::CoreAPI::get_assetmanager();
    if ( auto opt = asset_manager->require_asset<IE::Texture2D>( "gui/plan.png" ) ) {
        m_BuildingMenuIcon = opt.value().get();
    }
}

auto GUI::create( SampleProject* parent, IE::Ref<World> game_world ) -> IE::Own<GUI>
{
    return IE::Own<GUI>( new GUI( parent, game_world ) );
}

GUI::~GUI()
{
}

void GUI::render( float interp_factor, IE::GPURenderer* renderer )
{
    const IE::RenderContext* gui_render_ctx = renderer->acquire_rendercontext( m_UIRenderContextHandle );
    if ( gui_render_ctx == nullptr )
        return;

    gui_render_ctx->add_textured_quad( m_BuildingMenuIcon, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 200.0f }, IE::Origin::TopLeft, { 0.1f, 0.1f } );
}

bool GUI::handle_event( const SDL_Event& event )
{
    return false;
}

void GUI::update( double delta_time )
{
}
