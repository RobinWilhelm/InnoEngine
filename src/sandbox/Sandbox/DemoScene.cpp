#include "DemoScene.h"
#include "InnoEngine/Renderer.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/OrthographicCamera.h"

#include "InnoEngine/AssetManager.h"

#include <chrono>
#include <thread>
#include <optional>

DemoLayer::DemoLayer()
{
    // auto fontOpt = IE::CoreAPI::get_assetmanager()->require_asset<IE::Font>( "Calibri.ttf", true );
    // m_testFont   = fontOpt.value().get();

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
        m_colors[ i ]         = DXSM::Color( SDL_randf(), SDL_randf(), SDL_randf(), SDL_randf() );
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

        m_colors[ i ].x += delta_time * 0.01f;
        if ( m_colors[ i ].x > 1 )
            m_colors[ i ].x -= 1;
        m_colors[ i ].y += delta_time * 0.01f;
        if ( m_colors[ i ].y > 1 )
            m_colors[ i ].y -= 1;
        m_colors[ i ].z += delta_time * 0.01f;
        if ( m_colors[ i ].z > 1 )
            m_colors[ i ].z -= 1;
        // m_colors[i].w += delta_time * 0.1f;
        // if (m_colors[i].w > 1)
        // m_colors[i].w -= 1;
    }
}

void DemoLayer::render( float interp_factor, IE::GPURenderer* renderer )
{
    (void)interp_factor;
    renderer->set_view_projection( IE::CoreAPI::get_camera()->get_viewprojectionmatrix() );
    renderer->set_clear_color( { 0.0f, 0.0f, 0.0f, 1.0f } );

    renderer->register_texture( m_testTexture );

    for ( int i = 0; i < sprite_count; ++i ) {
        // m_testSprite.set_position( m_positions[ i ], IE::Sprite::Origin::Middle );
        // m_testSprite.set_scale( { m_scales[ i ], m_scales[ i ] } );
        // m_testSprite.set_rotation( m_rotations[ i ] );
        // m_testSprite.set_color( m_colors[ i ] );
        // renderer->add_sprite( m_testSprite );
        renderer->add_texture( m_testTexture, m_positions[ i ].x, m_positions[ i ].y, 0, m_rotations[ i ], m_colors[ i ], m_scales[ i ] );
    }
}

bool DemoLayer::handle_event( const SDL_Event& event )
{
    (void)event;
    return false;
}