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

    RenderContext::RenderContext( const RenderContext& other )
    {
        m_Renderer                 = other.m_Renderer;
        m_Specs                    = other.m_Specs;
        m_RenderCommandBufferIndex = other.m_RenderCommandBufferIndex;
        m_RenderCommandBuffer      = other.m_RenderCommandBuffer;
    }

    RenderContext RenderContext::operator=( const RenderContext& other )
    {
        m_Renderer                 = other.m_Renderer;
        m_Specs                    = other.m_Specs;
        m_RenderCommandBufferIndex = other.m_RenderCommandBufferIndex;
        m_RenderCommandBuffer      = other.m_RenderCommandBuffer;
        return *this;
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

    void RenderContext::add_clear( DXSM::Color clear_color )
    {
        m_ClearColor = clear_color;
    }

    void RenderContext::add_sprite( const Sprite& sprite ) const
    {
        IE_ASSERT( sprite.m_Texture != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( sprite.m_Texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_texture( sprite.m_Texture );

        Sprite2DPipeline::Command& cmd = m_RenderCommandBuffer->SpriteRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.TextureIndex   = sprite.m_Texture->m_RenderCommandBufferIndex;
        cmd.Size           = sprite.m_Size;
        cmd.Position       = sprite.m_RenderPosition;
        cmd.SourceRect     = sprite.m_SourceRect;
        cmd.Rotation       = sprite.m_RotationRadians;
        cmd.RotationOrigin = sprite.m_RotationOffset;
        cmd.Color          = sprite.m_Color;
    }

    void RenderContext::add_pixel( const DXSM::Vector2& position, const DXSM::Color& color ) const
    {
        add_quad( position, Origin::TopLeft, { 1.0f, 1.0f }, color );
    }

    void RenderContext::add_quad( const DXSM::Vector2& position, Origin position_origin, const DXSM::Vector2& size, const DXSM::Color& color, float rotation, const DXSM::Vector2& rotation_origin ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( color.A() == 1.0f ) {
            Primitive2DPipeline::QuadCommand& cmd = m_RenderCommandBuffer->QuadRenderCommandsOpaque.emplace_back();
            populate_command_base( &cmd );
            DXSM::Vector2 rotation_offset = rotation_origin * cmd.Size;
            cmd.Position                  = origin_transform( position_origin, position, size, rotation_offset );
            cmd.Size                      = size;
            cmd.Rotation                  = DirectX::XMConvertToRadians( rotation );
            cmd.RotationOrigin            = rotation_offset;
            cmd.Color                     = color;
        }
        else {
            Primitive2DPipeline::QuadCommand& cmd = m_RenderCommandBuffer->QuadRenderCommands.emplace_back();
            populate_command_base( &cmd );
            DXSM::Vector2 rotation_offset = rotation_origin * cmd.Size;
            cmd.Position                  = origin_transform( position_origin, position, size, rotation_offset );
            cmd.Size                      = size;
            cmd.Rotation                  = DirectX::XMConvertToRadians( rotation );
            cmd.RotationOrigin            = rotation_offset;
            cmd.Color                     = color;
        }
    }

    void RenderContext::add_line( const DXSM::Vector2& start_position, const DXSM::Vector2& end_position, const DXSM::Color& color, float thickness, float edge_fade ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        Primitive2DPipeline::LineCommand& cmd = m_RenderCommandBuffer->LineRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.Start     = start_position;
        cmd.End       = end_position;
        cmd.Thickness = thickness;
        cmd.EdgeFade  = edge_fade;
        cmd.Color     = color;
    }

    void RenderContext::add_lines( const std::vector<DXSM::Vector2>& points, const DXSM::Color& color, float thickness, float edge_fade, bool loop ) const
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

    void RenderContext::add_circle( const DXSM::Vector2& center_position, float radius, const DXSM::Color& color, float thickness, float edge_fade ) const
    {
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        Primitive2DPipeline::CircleCommand& cmd = m_RenderCommandBuffer->CircleRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.Position.x = center_position.x - radius;
        cmd.Position.y = center_position.y - radius;
        cmd.Color      = color;
        cmd.Radius     = radius;
        cmd.Fade       = edge_fade;
        cmd.Thickness  = thickness;
    }

    void RenderContext::add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, Origin position_origin, const DXSM::Vector2& scale, float rotation, const DXSM::Vector2& rotation_origin, const DXSM::Color& color ) const
    {
        IE_ASSERT( texture != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_texture( texture );

        Sprite2DPipeline::Command& cmd = m_RenderCommandBuffer->SpriteRenderCommands.emplace_back();
        populate_command_base( &cmd );
        cmd.TextureIndex = texture->m_RenderCommandBufferIndex;
        cmd.Size         = { scale.x * texture->m_Specs.Width * ( source_rect.z - source_rect.x ), scale.y * texture->m_Specs.Height * ( source_rect.w - source_rect.y ) };

        DXSM::Vector2 rotation_offset = rotation_origin * cmd.Size;
        cmd.Position                  = origin_transform( position_origin, position, cmd.Size, rotation_offset );
        cmd.SourceRect                = source_rect;
        cmd.Rotation                  = DirectX::XMConvertToRadians( rotation );
        cmd.RotationOrigin            = rotation_offset;
        cmd.Color                     = color;
    }

    void RenderContext::add_textured_quad_opaque( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, Origin position_origin, const DXSM::Vector2& scale, float rotation, const DXSM::Vector2& rotation_origin, const DXSM::Color& color ) const
    {
        IE_ASSERT( texture != nullptr );
        IE_ASSERT( m_RenderCommandBuffer != nullptr && m_RenderCommandBufferIndex != InvalidRenderCommandBufferIndex );

        if ( texture->m_RenderCommandBufferIndex == InvalidRenderCommandBufferIndex )
            register_texture( texture );

        Sprite2DPipeline::Command& cmd = m_RenderCommandBuffer->SpriteRenderCommandsOpaque.emplace_back();
        populate_command_base( &cmd );
        cmd.TextureIndex = texture->m_RenderCommandBufferIndex;
        cmd.Size         = { scale.x * texture->m_Specs.Width * ( source_rect.z - source_rect.x ), scale.y * texture->m_Specs.Height * ( source_rect.w - source_rect.y ) };

        DXSM::Vector2 rotation_offset = rotation_origin * cmd.Size;
        cmd.Position                  = origin_transform( position_origin, position, cmd.Size, rotation_offset );
        cmd.SourceRect                = source_rect;
        cmd.Rotation                  = DirectX::XMConvertToRadians( rotation );
        cmd.RotationOrigin            = rotation_offset;
        cmd.Color                     = color;
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
