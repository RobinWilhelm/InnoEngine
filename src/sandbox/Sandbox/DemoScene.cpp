#include "DemoScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"
#include "InnoEngine/graphics/DefaultCameraController.h"

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
        m_positions[ i ]      = DXSM::Vector2( SDL_randf() * 5000 - 2500, SDL_randf() * 2500 - 1250);
        m_rotationSpeeds[ i ] = SDL_randf() * 2 - 1;
        m_colors[ i ]         = DXSM::Color( SDL_randf(), SDL_randf(), SDL_randf(), max( 0.7f, SDL_randf() ) );
        m_scales[ i ]         = SDL_randf() * 2;
    }

    DXSM::Vector2 viewport_size( 1280, 720 );
    IE::Window*   window = IE::CoreAPI::get_gpurenderer()->get_window();
    m_SceneCamera        = IE::OrthographicCamera::create( { viewport_size.x, viewport_size.y } );
    m_SceneViewport      = IE::Viewport( window->width() / 2.0f - viewport_size.x / 2, 50, viewport_size.x, viewport_size.y );
    m_Parent->register_camera( m_SceneCamera );

    m_SceneCameraController = IE::DefaultCameraController::create( m_SceneCamera, m_SceneViewport );
    m_Parent->register_cameracontroller( m_SceneCameraController );

    m_UICamera = IE::OrthographicCamera::create( { static_cast<float>( window->width() ), static_cast<float>( window->height() ) } );
    m_Parent->register_camera( m_UICamera );
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
    renderer->register_texture(m_testTexture);
    renderer->register_font(m_testFont);

    renderer->use_camera( m_UICamera );
    renderer->use_view_port( m_Parent->get_fullscreen_viewport() );


    renderer->add_text_centered(m_testFont.get(), {1920 / 2.0f, 50.0f}, 40, "InnoEngine Demoscene", m_textColor);
    static DXSM::Vector4 viewport_bounding_box( m_SceneViewport.LeftOffset,
                                         m_SceneViewport.TopOffset + m_SceneViewport.Height,
                                         m_SceneViewport.LeftOffset + m_SceneViewport.Width,
                                         m_SceneViewport.TopOffset );

    renderer->add_bounding_box( { viewport_bounding_box }, { 0, 0 }, { 1.0f, 1.0f, 1.0f, 1.0f } );

    renderer->use_camera( m_SceneCamera );
    renderer->use_view_port( m_SceneViewport );

    // renderer->set_clear_color( { 0.09f, 0.42f, 0.09f, 1.0f } );
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );


    for ( int i = 0; i < 1000; ++i ) {
        renderer->add_circle( m_positions[ i ], m_scales[ i ] * 20, 0.00f, m_colors[ i ] );
    }

    float       pos_x = m_SceneViewport.Width / 2;
    float       pos_y = 100;
    const char* text  = "Lorem ipsum bla bla";

    for ( int i = 0; i < 1; ++i ) {
        renderer->add_text_centered( m_testFont.get(), { pos_x, pos_y }, 80, text, m_textColor );
        // renderer->add_bounding_box( aabb, { pos_x, pos_y }, { 0.0f, 1.0f, 0.0f, 1.0f } );
    }
    renderer->end_collection();
}

bool Demoscene::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
