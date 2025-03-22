#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Camera.h"
#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/Sprite.h"

namespace InnoEngine
{
    class GPURenderer;
    struct RenderCommandBuffer;

    class RenderContext : public std::enable_shared_from_this<RenderContext>
    {
        friend class GPURenderer;
        RenderContext() = default;

    public:
        static auto create( GPURenderer* renderer, Ref<Camera> camera, const Viewport& view_port ) -> Ref<RenderContext>;

        void            bind();
        Ref<Camera>     get_camera() const;
        const Viewport& get_viewport() const;
        void            set_viewport( const Viewport& viewport );

        void add_sprite( const Sprite& sprite );
        void add_pixel( const DXSM::Vector2& position, const DXSM::Color& color );
        void add_quad( const DXSM::Vector2& position, const DXSM::Vector2& size, float rotation, const DXSM::Color& color );
        void add_line( const DXSM::Vector2& start, const DXSM::Vector2& end, float thickness, float edge_fade, const DXSM::Color& color );
        void add_lines( const std::vector<DXSM::Vector2>& points, float thickness, float edge_fade, const DXSM::Color& color, bool loop );
        void add_circle( const DXSM::Vector2& position, float radius, float edge_fade, const DXSM::Color& color );
        void add_circle( const DXSM::Vector2& position, float radius, float thickness, float edge_fade, const DXSM::Color& color );

        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color );

        void add_text( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color );
        void add_text_centered( const Ref<Font> font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color );    // recalculates text size every time
        void add_imgui_draw_data( ImDrawData* draw_data );

        static uint16_t get_current_depth_layer();
        static uint16_t next_depth_layer();
        static void     use_specific_depth_layer( uint16_t layer );

    private:
        void         register_texture( Ref<Texture2D> texture );
        void         register_font( Ref<Font> font );
        void         populate_command_base( RenderCommandBase* cmd_base ) const;
        static float transform_layer_to_depth( uint16_t layer );

    private:
        GPURenderer* m_Renderer = nullptr;
        Ref<Camera>  m_Camera;
        Viewport     m_Viewport;

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
        RenderCommandBuffer*         m_RenderCommandBuffer      = nullptr;    // instance owned by GPURenderer

        static uint16_t m_CurrentDepthLayer;
        static float    m_CurrentLayerDepth;
    };
}    // namespace InnoEngine
