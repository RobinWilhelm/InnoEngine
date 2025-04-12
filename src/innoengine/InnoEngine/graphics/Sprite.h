#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Texture2D.h"

namespace InnoEngine
{
    class GPURenderer;

    class Sprite
    {
        friend class RenderContext;

    public:
        Sprite() = default;

        Sprite( Ref<Texture2D> texture ) :
            m_Texture( texture )
        {
            IE_ASSERT( m_Texture != nullptr );
        };

        int width() const;
        int height() const;

        void set_texture( const Ref<Texture2D> texture, const DXSM::Vector4& source_rect = { 0.0f, 0.0f, 1.0f, 1.0f } );
        void set_source_rect( const DXSM::Vector4& source_rect );    // source area of the texture

        void set_position( const DXSM::Vector2& position );
        void set_position_origin( Origin origin );

        void set_color( const DXSM::Color& ForegroundColor );
        void set_rotation( float degrees );
        void set_rotation_origin( const DXSM::Vector2& rotation_origin );
        void set_scale( const DXSM::Vector2&& scale );

    private:
        GPURenderer*   m_Renderer        = nullptr;
        Ref<Texture2D> m_Texture         = nullptr;
        DXSM::Vector4  m_SourceRect      = { 0.0f, 0.0f, 1.0f, 1.0f };
        Origin         m_Origin          = Origin::BottomLeft;
        DXSM::Vector2  m_Position        = { 0, 0 };
        DXSM::Color    m_Color           = { 1.0f, 1.0f, 1.0f, 1.0f };
        DXSM::Vector2  m_Scale           = { 1.0f, 1.0f };
        float          m_RotationRadians = 0.0f;
        DXSM::Vector2  m_RotationOrigin  = { 0.5f, 0.5f };

        DXSM::Vector2  m_RotationOffset  = {0.0f, 0.0f};           
        DXSM::Vector2  m_RenderPosition  = {0, 0};
        DXSM::Vector2  m_Size            = {0.0f, 0.0f};
    };

}    // namespace InnoEngine
