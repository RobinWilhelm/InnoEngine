#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Sprite.h"

#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/Renderer.h"

namespace InnoEngine
{
    int Sprite::width() const
    {
        return m_texture->get_specs().Width;
    }

    int Sprite::height() const
    {
        return m_texture->get_specs().Height;
    }

    void Sprite::set_texture( const Ref<Texture2D> texture, const DXSM::Vector4& source_rect )
    {
        IE_ASSERT( texture != nullptr );
        m_texture = texture;
        set_source_rect( source_rect );
    }

    void Sprite::set_source_rect( const DXSM::Vector4& source_rect )
    {
        m_sourceRect = source_rect;
    }

    void Sprite::set_position( const DXSM::Vector2& position, Origin origin )
    {
        m_position = position;
        switch ( origin ) {
        case InnoEngine::Origin::Middle:
            m_originOffset = { 0.5f, 0.5f };
            break;
        case InnoEngine::Origin::TopLeft:
            m_originOffset = { 0.0f, 0.0f };
            break;
        case InnoEngine::Origin::TopRight:
            m_originOffset = { 1.0f, 0.0f };
            break;
        case InnoEngine::Origin::BottomLeft:
            m_originOffset = { 0.0f, 1.0f };
            break;
        case InnoEngine::Origin::BottomRight:
            m_originOffset = { 1.0f, 1.0f };
            break;
        }
    }

    void Sprite::set_color( const DXSM::Color& ForegroundColor )
    {
        m_color = ForegroundColor;
    }

    void Sprite::set_rotation( float degrees )
    {
        m_rotationDegrees = degrees;
    }

    void Sprite::set_scale( const DXSM::Vector2&& scale )
    {
        m_scale = scale;
    }
}    // namespace InnoEngine
