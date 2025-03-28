#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Camera.h"
#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/Sprite.h"

namespace InnoEngine
{
    class Font;
    class Texture2D;
    class GPURenderer;
    struct RenderContextCommands;

    struct RenderContextSpecifications
    {
        Ref<Texture2D> ColorTarget;
        Ref<Camera>    Camera;
        Viewport       Viewport;
    };

    class RenderContext
    {
        friend class GPURenderer;
        RenderContext() = default;
        static auto create( GPURenderer* renderer, const RenderContextSpecifications& spec ) -> Ref<RenderContext>;

    public:
        Ref<Camera>     get_camera() const;
        const Viewport& get_viewport() const;

        void add_sprite( const Sprite& sprite ) const;
        void add_pixel( const DXSM::Vector2& position, const DXSM::Color& color ) const;
        void add_quad( Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& size, float rotation, const DXSM::Color& color ) const;
        void add_line( const DXSM::Vector2& start, const DXSM::Vector2& end, float thickness, float edge_fade, const DXSM::Color& color ) const;
        void add_lines( const std::vector<DXSM::Vector2>& points, float thickness, float edge_fade, const DXSM::Color& color, bool loop ) const;
        void add_circle( const DXSM::Vector2& position, float radius, float edge_fade, const DXSM::Color& color ) const;
        void add_circle( const DXSM::Vector2& position, float radius, float thickness, float edge_fade, const DXSM::Color& color ) const;

        void add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector2& position ) const;
        void add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color ) const;
        void add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector4& source_rect, const DXSM::Vector2& position ) const;
        void add_textured_quad( Ref<Texture2D> texture, Origin origin, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color ) const;

        void add_text( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color ) const;
        void add_text_centered( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color ) const;    // recalculates text size every time

        static uint16_t get_current_depth_layer();
        static uint16_t next_depth_layer();
        static void     use_specific_depth_layer( uint16_t layer );

    private:
        void         register_texture( Ref<Texture2D> texture ) const;
        void         register_font( Ref<Font> font ) const;
        void         populate_command_base( RenderCommandBase* cmd_base ) const;
        static float transform_layer_to_depth( uint16_t layer );

        const DXSM::Vector2 origin_transform( Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& size ) const;

    private:
        GPURenderer*                m_Renderer = nullptr;
        RenderContextSpecifications m_Specs    = {};

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
        RenderContextCommands*  m_RenderCommandBuffer      = nullptr;    // instance owned by GPURenderer

        DXSM::Color m_CustomColorTargetClearColor = {};
        bool        m_ClearCustomColorTarget      = false;

        static uint16_t m_CurrentDepthLayer;
        static float    m_CurrentLayerDepth;
    };

    using RenderContextHandle = uint32_t; 
}    // namespace InnoEngine
