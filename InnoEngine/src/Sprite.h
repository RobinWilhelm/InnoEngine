#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "BaseTypes.h"
#include "Texture2D.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

namespace InnoEngine
{
    class GPURenderer;

    class Sprite
    {
        friend class GPURenderer;

    public:
        Sprite() = default;

        Sprite( Ref<Texture2D> texture ) :
            m_texture( texture )
        {
            IE_ASSERT( m_texture != nullptr );
        };

        SDL_PixelFormat get_format() const;
        int             width() const;
        int             height() const;

        void set_texture( const Ref<Texture2D> texture, const DXSM::Vector4&& source_rect );
        void set_source_rect( const DXSM::Vector4&& source_rect );    // source area of the texture
        void set_position( const DXSM::Vector2&& position );
        void set_layer( uint16_t layer );
        void set_color( const DXSM::Color&& color );
        void set_rotation( float rotation );
        void set_scale( const DXSM::Vector2&& scale );

        void render();

    private:
        GPURenderer*   m_renderer   = nullptr;
        Ref<Texture2D> m_texture    = nullptr;
        DXSM::Vector4  m_sourceRect = { 0.0f, 0.0f, 1.0f, 1.0f };
        DXSM::Vector2  m_position   = { 0, 0 };
        DXSM::Color    m_color      = { 1.0f, 1.0f, 1.0f, 1.0f };
        DXSM::Vector2  m_scale      = { 1.0f, 1.0f };
        float          m_rotation   = 0.0f;
        uint16_t       m_layer      = 0;
    };

}    // namespace InnoEngine
