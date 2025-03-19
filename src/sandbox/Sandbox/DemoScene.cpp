#include "DemoScene.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/OrthographicCamera.h"

#include "InnoEngine/AssetManager.h"

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
    for ( int i = 0; i < sprite_count; ++i ) {
        m_rotations[ i ] += m_rotationSpeeds[ i ] * 360.0f * delta_time;
        if ( m_rotations[ i ] >= 360.0f )
            m_rotations[ i ] -= 360.0f;
    }

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
    renderer->set_clear_color( { 0.09f, 0.42f, 0.09f, 1.0f } );
    // renderer->set_clear_color({1.0f, 1.0f, 1.0f, 1.0f});

    renderer->register_texture( m_testTexture );

    renderer->register_font( m_testFont );

    uint32_t count = 0;
    for ( int i = 0; i < 100; ++i ) {
        // renderer->add_pixel( m_positions[ i ], m_colors[ i ] );
        renderer->add_circle({0, 0}, 100, 0.01f, {0.0f, 1.0f, 0.0f, 1.0f});
    }

    static std::vector<DXSM::Vector2> points {
        { 200.0f, 100.0f },
        { 600.0f, 200.0f },
        { 500.0f, 600.0f },
        { 200.0f, 600.0f }
    };
    //renderer->add_lines( points, 40, 0.1f, { 0.0f, 1.0f, 0.0f, 1.0f }, true );
    //renderer->next_layer();
    //renderer->add_circle( { 600, 400 }, 300.0f, 5.0f, 0.01f, { 0.5f, 0.5f, 0.5f, 1.0f } );
}

bool DemoLayer::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}
