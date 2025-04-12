#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Sprite.h"

#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/Renderer.h"

namespace InnoEngine
{
    int Sprite::width() const
    {
        return m_Texture->get_specs().Width;
    }

    int Sprite::height() const
    {
        return m_Texture->get_specs().Height;
    }

    void Sprite::set_texture( const Ref<Texture2D> texture, const DXSM::Vector4& source_rect )
    {
        IE_ASSERT( texture != nullptr );
        m_Texture = texture;
        set_source_rect( source_rect );
    }

    void Sprite::set_source_rect( const DXSM::Vector4& source_rect )
    {
        m_SourceRect     = source_rect;
        m_Size.x         = m_Scale.x * m_Texture->get_specs().Width * ( m_SourceRect.z - m_SourceRect.x );
        m_Size.y         = m_Scale.y * m_Texture->get_specs().Height * ( m_SourceRect.w - m_SourceRect.y );
        m_RotationOffset = m_RotationOrigin * m_Size;
        m_RenderPosition = origin_transform( m_Origin, m_Position, m_Size, m_RotationOffset );
    }

    void Sprite::set_position( const DXSM::Vector2& position )
    {
        m_Position       = position;
        m_RenderPosition = origin_transform( m_Origin, m_Position, m_Size, m_RotationOffset );
    }

    void Sprite::set_position_origin( Origin origin )
    {
        m_Origin = origin;
    }

    void Sprite::set_color( const DXSM::Color& ForegroundColor )
    {
        m_Color = ForegroundColor;
    }

    void Sprite::set_rotation( float degrees )
    {
        m_RotationRadians = DirectX::XMConvertToRadians( degrees );
    }

    void Sprite::set_rotation_origin( const DXSM::Vector2& rotation_origin )
    {
        m_RotationOrigin = rotation_origin;
        m_RotationOffset = m_RotationOrigin * m_Size;
        m_RenderPosition = origin_transform( m_Origin, m_Position, m_Size, m_RotationOffset );
    }

    void Sprite::set_scale( const DXSM::Vector2&& scale )
    {
        m_Scale          = scale;
        m_Size.x         = m_Scale.x * m_Texture->get_specs().Width * ( m_SourceRect.z - m_SourceRect.x );
        m_Size.y         = m_Scale.y * m_Texture->get_specs().Height * ( m_SourceRect.w - m_SourceRect.y );
        m_RotationOffset = m_RotationOrigin * m_Size;
        m_RenderPosition = origin_transform( m_Origin, m_Position, m_Size, m_RotationOffset );
    }
}    // namespace InnoEngine
