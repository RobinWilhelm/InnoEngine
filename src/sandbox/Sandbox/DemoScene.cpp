#include "DemoScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Window.h"

#include <chrono>
#include <thread>
#include <optional>

DemoLayer::DemoLayer()
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
        m_positions[ i ]      = DXSM::Vector2( SDL_randf() * 1920, SDL_randf() * 1080 );
        m_rotationSpeeds[ i ] = SDL_randf() * 2 - 1;
        m_colors[ i ]         = DXSM::Color( SDL_randf(), SDL_randf(), SDL_randf(), max( 0.7f, SDL_randf() ) );
        m_scales[ i ]         = SDL_randf() * 2;
    }
}

void DemoLayer::update( double delta_time )
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

void DemoLayer::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->set_view_projection( IE::CoreAPI::get_camera()->get_viewprojectionmatrix() );
    //renderer->set_clear_color( { 0.09f, 0.42f, 0.09f, 1.0f } );
     renderer->set_clear_color({0.0f, 0.0f, 0.0f, 1.0f});

    renderer->register_texture( m_testTexture );

    renderer->register_font( m_testFont );
    /*
    for ( int i = 0; i < 100000; ++i ) {
       // renderer->add_textured_quad(m_testTexture, m_positions[ i ], { m_scales[ i ] , m_scales[ i ]  }, m_rotations[ i ], m_colors[ i ] );
        renderer->add_circle(m_positions[i], m_scales[i] * 20, 0.02f, m_colors[i]);
    }
      */


    renderer->add_quad({500, 100}, {100, 20}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    renderer->add_quad({500, 900}, {100, 20}, 0.0f, {0.0f, 0.0f, 0.0f, 1.0f});

    float       pos_x = renderer->get_window()->width() / 2;
    float       pos_y = renderer->get_window()->height() / 2;
    const char* text  = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\nDuis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\nExcepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    DXSM::Vector4 aabb = m_testFont->get_aabb( 30, text );

    float text_width  = aabb.z - aabb.x;
    float text_height = aabb.y - aabb.w;
    pos_x -= text_width / 2;
    pos_y -= text_height / 2;
    for ( int i = 0; i < 1; ++i ) {
        renderer->add_text( m_testFont.get(), { pos_x, pos_y }, 80, text, {1.0f, 1.0f, 1.0f, 1.0f});
        //renderer->add_bounding_box( aabb, { pos_x, pos_y }, { 0.0f, 1.0f, 0.0f, 1.0f } );
    }
    
}

bool DemoLayer::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
