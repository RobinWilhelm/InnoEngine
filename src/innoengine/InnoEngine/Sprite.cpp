#include "iepch.h"
#include "Sprite.h"

#include "Texture2D.h"

#include "Renderer.h"

namespace InnoEngine
{
    SDL_PixelFormat Sprite::get_format() const
    {
        return m_texture->get_format();
    }

    int Sprite::width() const
    {
        return m_texture->width();
    }

    int Sprite::height() const
    {
        return m_texture->height();
    }

    void Sprite::set_texture( const Ref<Texture2D> texture, const DXSM::Vector4&& source_rect )
    {
        IE_ASSERT( texture != nullptr );
        m_texture = texture;
        set_source_rect( std::forward<const DXSM::Vector4>( source_rect ) );
    }

    void Sprite::set_source_rect( const DXSM::Vector4&& source_rect )
    {
        m_sourceRect = source_rect;
    }

    void Sprite::set_position(const DXSM::Vector2& position, Origin origin)
    {
        m_position = position;
        switch (origin)
        {
            case InnoEngine::Sprite::Origin::Middle:
                m_originOffset = {0.5f, 0.5f};
                break;
            case InnoEngine::Sprite::Origin::TopLeft:
                m_originOffset = {0.0f, 0.0f};
                break;
            case InnoEngine::Sprite::Origin::TopRight:
                m_originOffset = {1.0f, 0.0f};
                break;
            case InnoEngine::Sprite::Origin::BottomLeft:
                m_originOffset = {0.0f, 1.0f};
                break;
            case InnoEngine::Sprite::Origin::BottomRight:
                m_originOffset = {1.0f, 1.0f};
                break;
        }
    }

    void Sprite::set_position( const DXSM::Vector2&& position, Origin origin)
    {
        m_position = position;
        switch (origin)
        {
            case InnoEngine::Sprite::Origin::Middle:
                m_originOffset = {0.5f, 0.5f};
                break;
            case InnoEngine::Sprite::Origin::TopLeft:
                m_originOffset = {0.0f, 0.0f};
                break;
            case InnoEngine::Sprite::Origin::TopRight:
                m_originOffset = {1.0f, 0.0f};
                break;
            case InnoEngine::Sprite::Origin::BottomLeft:
                m_originOffset = {0.0f, 1.0f};
                break;
            case InnoEngine::Sprite::Origin::BottomRight:
                m_originOffset = {1.0f, 1.0f};
                break;
        }
    }

    void Sprite::set_layer( uint16_t layer )
    {
        m_layer = layer;
    }

    void Sprite::set_color(const DXSM::Color& color)
    {
        m_color = color;
    }

    void Sprite::set_color( const DXSM::Color&& color )
    {
        m_color = color;
    }

    void Sprite::set_rotation( float degrees )
    {
        m_rotationDegrees = degrees;
    }

    void Sprite::set_scale( const DXSM::Vector2&& scale )
    {
        m_scale = scale;
    }

    void Sprite::render()
    {
        IE_ASSERT( m_texture != nullptr );
        static GPURenderer* renderer = CoreAPI::get_gpurenderer();
        IE_ASSERT( renderer != nullptr );
        renderer->add_sprite( *this );
    }
}    // namespace InnoEngine
