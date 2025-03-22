#include "DemoScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"
#include "InnoEngine/graphics/DefaultCameraController.h"
#include "InnoEngine/graphics/RenderContext.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Window.h"

#include "Sandbox/Sandbox.h"

#include <chrono>
#include <thread>
#include <optional>

Demoscene::Demoscene( Sandbox* parent ) :
    IE::Layer( parent )
{
    auto fontOpt = IE::CoreAPI::get_assetmanager()->require_asset<IE::Font>( "Calibri.ttf", true );
    m_testFont   = fontOpt.value().get();

    auto textureOpt = IE::CoreAPI::get_assetmanager()->require_asset<IE::Texture2D>( "tile.png", true );
    m_testTexture   = textureOpt.value().get();
    m_testSprite.set_texture( m_testTexture );

    m_positions.resize( sprite_count );
    m_rotations.resize( sprite_count );
    m_colors.resize( sprite_count );
    m_rotationSpeeds.resize( sprite_count );
    m_scales.resize( sprite_count );

    for ( int i = 0; i < sprite_count; ++i ) {
        m_positions[ i ]      = DXSM::Vector2( SDL_randf() * 5000 - 2500, SDL_randf() * 2500 - 1250 );
        m_rotationSpeeds[ i ] = SDL_randf() * 2 - 1;
        m_colors[ i ]         = DXSM::Color( SDL_randf(), SDL_randf(), SDL_randf(), max( 0.7f, SDL_randf() ) );
        m_scales[ i ]         = SDL_randf() * 2;
    }

    DXSM::Vector2 viewport_size( 1280, 720 );
    IE::Window*   window = IE::CoreAPI::get_gpurenderer()->get_window();

    auto scene_camera   = IE::OrthographicCamera::create( { viewport_size.x, viewport_size.y } );
    auto scene_viewport = IE::Viewport( window->width() / 2.0f - viewport_size.x / 2, 50, viewport_size.x, viewport_size.y );
    m_Parent->register_camera( scene_camera );

    auto scene_camera_controller = IE::DefaultCameraController::create( scene_camera, scene_viewport );
    m_Parent->register_cameracontroller( scene_camera_controller );

    m_SceneRenderCtx = IE::RenderContext::create( parent->get_renderer(), scene_camera, scene_viewport );
}

void Demoscene::update( double delta_time )
{
    //
    // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    /*
   for ( int i = 0; i < sprite_count; ++i ) {
       m_rotations[ i ] += m_rotationSpeeds[ i ] * 360.0f * delta_time;
       if ( m_rotations[ i ] >= 360.0f )
           m_rotations[ i ] -= 360.0f;
   }
   */
    uint64_t delta = IE::get_tick_count() - m_colorAnimStart;
    m_color_h      = static_cast<float>( delta ) / ( IE::TicksPerSecond * 3 );

    if ( delta >= IE::TicksPerSecond * 3 ) {
        m_colorAnimStart = IE::get_tick_count();
        m_color_h        = 0.0f;
    }

    ImGui::ColorConvertHSVtoRGB( m_color_h, m_color_s, m_color_v, m_textColor.x, m_textColor.y, m_textColor.z );
}

void Demoscene::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->begin_collection();
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    static auto render_ctx = m_Parent->get_default_rendercontext();
    render_ctx->bind();

    render_ctx->add_text_centered( m_testFont, { 1920 / 2.0f, 50.0f }, 40, "InnoEngine Demoscene", m_textColor );

    auto                 scene_viewport = m_SceneRenderCtx->get_viewport();
    static DXSM::Vector4 viewport_bounding_box( scene_viewport.LeftOffset,
                                                scene_viewport.TopOffset + scene_viewport.Height,
                                                scene_viewport.LeftOffset + scene_viewport.Width,
                                                scene_viewport.TopOffset );

    render_ctx->add_quad( { scene_viewport.LeftOffset + scene_viewport.Width / 2, scene_viewport.TopOffset + scene_viewport.Height / 2 },
                          { scene_viewport.Width, scene_viewport.Height },
                          0.0f,
                          { 0.0f, 1.0f, 0.0f, 1.0f } );
    /*
    render_ctx->add_bounding_box( { viewport_bounding_box }, { 0, 0 }, { 1.0f, 1.0f, 1.0f, 1.0f } );
    */
    m_SceneRenderCtx->bind();
    for ( int i = 0; i < 1000000; ++i ) {
        m_SceneRenderCtx->add_circle( m_positions[ i ], m_scales[ i ] * 20, 0.00f, m_colors[ i ] );
    }
    m_SceneRenderCtx->add_text_centered( m_testFont, { scene_viewport.Width / 2, 100 }, 80, "Lorem ipsum bla bla", m_textColor );

    renderer->end_collection();
}

bool Demoscene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
