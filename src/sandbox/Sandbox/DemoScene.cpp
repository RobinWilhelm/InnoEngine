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
        m_colors[ i ]         = DXSM::Color( SDL_randf(), SDL_randf(), SDL_randf(), /*max( 0.7f, SDL_randf() )*/ 1.0f );
        m_scales[ i ]         = SDL_randf() * 2;
    }

    DXSM::Vector2 viewport_size( 1280, 720 );
    IE::Window*   window = IE::CoreAPI::get_gpurenderer()->get_window();

    IE::TextureSpecifications spec = {};
    spec.EnableMipmap              = false;
    spec.Format                    = IE::TextureFormat::RGBX;
    spec.RenderTarget              = true;
    spec.Width                     = viewport_size.x;
    spec.Height                    = viewport_size.y;

    IE::RenderContextSpecifications render_ctx_scene = {};
    render_ctx_scene.ColorTarget                     = IE::Texture2D::create( spec ).value();
    render_ctx_scene.Camera                          = IE::OrthographicCamera::create( { viewport_size.x, viewport_size.y } );
    render_ctx_scene.Viewport                        = IE::Viewport( 0, 0, viewport_size.x, viewport_size.y );
    m_SceneCtxHandle                                 = m_Parent->get_renderer()->create_rendercontext( render_ctx_scene );

    m_Parent->register_camera( render_ctx_scene.Camera );

    auto scene_camera_controller = IE::DefaultCameraController::create( render_ctx_scene.Camera, render_ctx_scene.Viewport );
    m_Parent->register_cameracontroller( scene_camera_controller );
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
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    const IE::RenderContext* fullscreen_ctx = renderer->acquire_rendercontext( m_Parent->get_fullscreen_rch() );
    if ( fullscreen_ctx ) {
        //fullscreen_ctx->add_text_centered( m_testFont, { 1920 / 2.0f, 50.0f }, 40, "InnoEngine Demoscene", m_textColor );

        //fullscreen_ctx->add_quad(IE::Origin::TopLeft, {1920 / 2.0f - 1280 / 2.0f, 200}, {1280, 720}, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f});
        //fullscreen_ctx->add_quad(IE::Origin::TopLeft, {1920 / 2.0f - 1280 / 2.0f, 200}, {1280, 720}, 0.0f, {1.0f, 0.0f, 0.0f, 0.5f});
        //fullscreen_ctx->add_quad(IE::Origin::TopLeft, {1920 / 2.0f - 1280 / 2.0f, 200}, {1280, 720}, 0.0f, {0.0f, 0.0f, 1.0f, 0.5f});
    }

    /*
    IE::RenderContext* scene_ctx = renderer->acquire_rendercontext( m_SceneCtxHandle );
    if ( scene_ctx ) {

        IE::RenderContext::next_depth_layer();
        scene_ctx->add_clear( { 0.0f, 0.0f, 0.0f, 1.0f } );

               IE::RenderContext::next_depth_layer();
        // fullscreen_ctx->add_textured_quad( scene_ctx->get_rendertarget(), IE::Origin::TopLeft, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1920 / 2.0f - scene_ctx->get_viewport().Width / 2.0f, 200 } );
    }
    */
}

bool Demoscene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
