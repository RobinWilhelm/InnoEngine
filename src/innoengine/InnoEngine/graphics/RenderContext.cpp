#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/RenderContext.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/graphics/RenderCommandBuffer.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"

namespace InnoEngine
{
    uint16_t RenderContext::m_CurrentDepthLayer = 0;
    float    RenderContext::m_CurrentLayerDepth = 0.0f;

    auto RenderContext::create( GPURenderer* renderer, const RenderContextSpecifications& specs ) -> Ref<RenderContext>
    {
        IE_ASSERT( renderer != nullptr );
        Ref<RenderContext> render_ctx = Ref<RenderContext>( new RenderContext() );
        render_ctx->m_Renderer        = renderer;
        render_ctx->m_Specs           = specs;
        return render_ctx;
    }

    Ref<Texture2D> RenderContext::get_rendertarget() const
    {
        return m_Specs.ColorTarget;
    }

    Ref<Camera> RenderContext::get_camera() const
    {
        return m_Specs.Camera;
    }

    const Viewport& RenderContext::get_viewport() const
    {
        return m_Specs.Viewport;
    }

    void RenderContext::add_sprite( const Sprite& sprite ) const
    {
        IE_ASSERT( sprite.m_texture != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( sprite.m_texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_texture( sprite.m_texture );

        Sprite2DPipeline::Command& cmd = m_RenderCommandBuffer->SpriteRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.TextureIndex = sprite.m_texture->m_RenderCommandBufferIndex;
        cmd.Position     = sprite.m_position;
        cmd.Size         = { sprite.m_scale.x * sprite.m_texture->m_Specs.Width, sprite.m_scale.y * sprite.m_texture->m_Specs.Height };
        cmd.OriginOffset = sprite.m_originOffset * cmd.Size;
        cmd.SourceRect   = sprite.m_sourceRect;
        cmd.Rotation     = DirectX::XMConvertToRadians( sprite.m_rotationDegrees );
        cmd.Color        = sprite.m_color;
    }

    void RenderContext::add_pixel( const DXSM::Vector2& position, const DXSM::Color& color ) const
    {
        add_quad( Origin::TopLeft, position, { 1.0f, 1.0f }, 0.0f, color );
    }

    void RenderContext::add_quad( Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& size, float rotation, const DXSM::Color& color ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        Primitive2DPipeline::QuadCommand& cmd = m_RenderCommandBuffer->QuadRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.Position = origin_transform( origin, position, size );
        cmd.Size     = size;
        cmd.Rotation = DirectX::XMConvertToRadians( rotation );
        cmd.Color    = color;
    }

    void RenderContext::add_line( const DXSM::Vector2& start, const DXSM::Vector2& end, float thickness, float edge_fade, const DXSM::Color& color ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        Primitive2DPipeline::LineCommand& cmd = m_RenderCommandBuffer->LineRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.Start     = start;
        cmd.End       = end;
        cmd.Thickness = thickness;
        cmd.EdgeFade  = edge_fade;
        cmd.Color     = color;
    }

    void RenderContext::add_lines( const std::vector<DXSM::Vector2>& points, float thickness, float edge_fade, const DXSM::Color& color, bool loop ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        uint32_t point_amount = static_cast<uint32_t>( points.size() );
        if ( point_amount == 1 )
            return;

        for ( size_t i = 0; i < point_amount; ++i ) {
            if ( i + 1 < point_amount ) {
                Primitive2DPipeline::LineCommand& cmd = m_RenderCommandBuffer->LineRenderCommands.emplace_back();
                populate_command_base( &cmd );
                cmd.Start     = points[ i ];
                cmd.End       = points[ i + 1 ];
                cmd.Color     = color;
                cmd.Thickness = thickness;
                cmd.EdgeFade  = edge_fade;
            }
            else if ( loop ) {
                Primitive2DPipeline::LineCommand& cmd = m_RenderCommandBuffer->LineRenderCommands.emplace_back();
                populate_command_base( &cmd );
                cmd.Start     = points[ i ];
                cmd.End       = points[ 0 ];
                cmd.Color     = color;
                cmd.Thickness = thickness;
                cmd.EdgeFade  = edge_fade;
            }
        }
    }

    void RenderContext::add_circle( const DXSM::Vector2& position, float radius, float edge_fade, const DXSM::Color& color ) const
    {
        add_circle( position, radius, radius, edge_fade, color );
    }

    void RenderContext::add_circle( const DXSM::Vector2& position, float radius, float thickness, float edge_fade, const DXSM::Color& color ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        Primitive2DPipeline::CircleCommand& cmd = m_RenderCommandBuffer->CircleRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.Position.x = position.x - radius;
        cmd.Position.y = position.y - radius;
        cmd.Color      = color;
        cmd.Radius     = radius;
        cmd.Fade       = edge_fade;
        cmd.Thickness  = thickness / radius;
    }

    void RenderContext::add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector2& position ) const
    {
        add_textured_quad( texture, origin, position, { 1.0f, 1.0f }, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f } );
    }

    void RenderContext::add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color ) const
    {
        add_textured_quad( texture, origin, { 0.0f, 0.0f, 1.0f, 1.0f }, position, scale, rotation, color );
    }

    void RenderContext::add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector4& source_rect, const DXSM::Vector2& position ) const
    {
        add_textured_quad( texture, origin, source_rect, position, { 1.0f, 1.0f }, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f } );
    }

    void RenderContext::add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color ) const
    {
        IE_ASSERT( texture != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_texture( texture );

        Sprite2DPipeline::Command& cmd = m_RenderCommandBuffer->SpriteRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.TextureIndex = texture->m_RenderCommandBufferIndex;
        cmd.Size         = { scale.x * texture->m_Specs.Width, scale.y * texture->m_Specs.Height };
        cmd.Position     = origin_transform( origin, position, cmd.Size );
        cmd.OriginOffset = cmd.Size * 0.5f;
        cmd.SourceRect   = source_rect;
        cmd.Rotation     = DirectX::XMConvertToRadians( rotation );
        cmd.Color        = color;
    }

    void RenderContext::add_text( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color ) const
    {
        IE_ASSERT( font != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( font->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_font( font );

        Font2DPipeline::Command& cmd = m_RenderCommandBuffer->FontRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.FontFBIndex     = font->m_RenderCommandBufferIndex;
        cmd.StringIndex     = m_RenderCommandBuffer->StringBuffer->insert( text );
        cmd.StringLength    = static_cast<uint32_t>( text.size() );
        cmd.Position        = position;
        cmd.FontSize        = text_size;
        cmd.ForegroundColor = color;
    }

    void RenderContext::add_text_centered( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color ) const
    {
        DXSM::Vector4 aabb        = font->get_aabb( text_size, text );
        float         text_width  = aabb.z - aabb.x;
        float         text_height = aabb.y - aabb.w;
        add_text( font, { position.x - text_width / 2, position.y - text_height / 2 }, text_size, text, color );
    }

    uint16_t RenderContext::get_current_depth_layer()
    {
        return m_CurrentDepthLayer;
    }

    uint16_t RenderContext::next_depth_layer()
    {
        m_CurrentLayerDepth = transform_layer_to_depth( ++m_CurrentDepthLayer );
        return m_CurrentDepthLayer;
    }

    void RenderContext::use_specific_depth_layer( uint16_t layer )
    {
        m_CurrentDepthLayer = layer;
        m_CurrentLayerDepth = transform_layer_to_depth( m_CurrentDepthLayer );
    }

    float RenderContext::transform_layer_to_depth( uint16_t layer )
    {
        return layer == 0 ? 0.0f : static_cast<float>( layer ) / 65536.0f;
    }

    const DXSM::Vector2 RenderContext::origin_transform( Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& size ) const
    {
        DXSM::Vector2 new_pos = position;
        switch ( origin ) {
        case InnoEngine::Origin::Middle:
            return new_pos;
            break;
        case InnoEngine::Origin::TopLeft:
            new_pos += size * 0.5f;
            break;
        case InnoEngine::Origin::TopRight:
            new_pos.x -= size.x * 0.5f;
            new_pos.y += size.y * 0.5f;
            break;
        case InnoEngine::Origin::BottomLeft:
            new_pos.x += size.x * 0.5f;
            new_pos.y -= size.y * 0.5f;
            break;
        case InnoEngine::Origin::BottomRight:
            new_pos -= size * 0.5f;
            break;
        default:
            IE_ASSERT( "Invalid origin" );
            break;
        }
        return new_pos;
    }

    void RenderContext::register_texture( Ref<Texture2D> texture ) const
    {
        IE_ASSERT( texture != nullptr && m_RenderCommandBuffer != nullptr );
        IE_ASSERT( texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex );

        texture->m_RenderCommandBufferIndex = static_cast<RenderCommandBufferIndexType>( m_RenderCommandBuffer->TextureRegister->size() );
        m_RenderCommandBuffer->TextureRegister->push_back( texture );
    }

    void RenderContext::register_font( Ref<Font> font ) const
    {
        IE_ASSERT( font != nullptr && m_RenderCommandBuffer != nullptr );
        IE_ASSERT( font->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex );

        font->m_RenderCommandBufferIndex = static_cast<RenderCommandBufferIndexType>( m_RenderCommandBuffer->FontRegister->size() );
        m_RenderCommandBuffer->FontRegister->push_back( font );
    }

    void RenderContext::populate_command_base( RenderCommandBase* cmd_base ) const
    {
        cmd_base->ContextIndex = m_RenderCommandBufferIndex;
        cmd_base->Depth        = m_CurrentLayerDepth;
    }
}    // namespace InnoEngine
