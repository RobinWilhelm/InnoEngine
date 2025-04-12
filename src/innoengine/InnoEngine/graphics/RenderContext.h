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

    // this is the per frame data that needs to be copied for the renderthread
    struct RenderContextFrameData
    {
        Ref<Texture2D> RenderTarget;    // a custom color target, if nullptr then the swapchain is the target
        DXSM::Color    ClearColor;      // only valid when a custom rendertarget is set, ignored otherwise

        DXSM::Matrix                 ViewProjectionMatrix;
        SDL_GPUViewport              Viewport;
        RenderCommandBufferIndexType Index;
    };

    class RenderContext
    {
        friend class GPURenderer;
        RenderContext() = default;
        static auto create( GPURenderer* renderer, const RenderContextSpecifications& spec ) -> Ref<RenderContext>;

        RenderContext( const RenderContext& other );
        RenderContext operator=( const RenderContext& other );

        RenderContext( const RenderContext&& other )           = delete;
        RenderContext operator=( const RenderContext&& other ) = delete;

    public:
        Ref<Texture2D>  get_rendertarget() const;
        Ref<Camera>     get_camera() const;
        const Viewport& get_viewport() const;

        void add_clear( DXSM::Color clear_color );

        void add_sprite( const Sprite& sprite ) const;
        void add_pixel( const DXSM::Vector2& position,
                        const DXSM::Color&   color ) const;

        void add_quad( const DXSM::Vector2& position,
                       Origin               position_origin,
                       const DXSM::Vector2& size,
                       const DXSM::Color&   color,
                       float                rotation        = 0.0f,
                       const DXSM::Vector2& rotation_origin = { 0.5, 0.5 } ) const;

        void add_line( const DXSM::Vector2& start_position,
                       const DXSM::Vector2& end_position,
                       const DXSM::Color&   color,
                       float                thickness = 1.0f,
                       float                edge_fade = 0.0f ) const;

        void add_lines( const std::vector<DXSM::Vector2>& points,
                        const DXSM::Color&                color,
                        float                             thickness = 1.0f,
                        float                             edge_fade = 0.0f,
                        bool                              loop      = false ) const;

        void add_circle( const DXSM::Vector2& center_position,
                         float                radius,
                         const DXSM::Color&   color,
                         float                thickness = 1.0f,
                         float                edge_fade = 0.0f ) const;

        void add_textured_quad( Ref<Texture2D>       texture,
                                const DXSM::Vector4& source_rect,
                                const DXSM::Vector2& position,
                                Origin               position_origin = Origin::TopLeft,
                                const DXSM::Vector2& scale           = { 1.0f, 1.0f },
                                float                rotation        = 0.0f,
                                const DXSM::Vector2& rotation_origin = { 0.5f, 0.5f },
                                const DXSM::Color&   color           = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;

        void add_textured_quad_opaque( Ref<Texture2D>       texture,
                                       const DXSM::Vector4& source_rect,
                                       const DXSM::Vector2& position,
                                       Origin               position_origin = Origin::TopLeft,
                                       const DXSM::Vector2& scale           = { 1.0f, 1.0f },
                                       float                rotation        = 0.0f,
                                       const DXSM::Vector2& rotation_origin = { 0.5f, 0.5f },
                                       const DXSM::Color&   color           = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;

        void add_text( const Ref<Font>      font,
                       const DXSM::Vector2& position,
                       uint32_t             text_size,
                       std::string_view     text,
                       const DXSM::Color&   color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;

        // Beware: recalculates text size every time. Its probably better to cache the text size for static texts.
        void add_text_centered( const Ref<Font>      font,
                                const DXSM::Vector2& position,
                                uint32_t             text_size,
                                std::string_view     text,
                                const DXSM::Color&   color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;

        static uint16_t get_current_depth_layer();
        static uint16_t next_depth_layer();
        static void     use_specific_depth_layer( uint16_t layer );

    private:
        void         register_texture( Ref<Texture2D> texture ) const;
        void         register_font( Ref<Font> font ) const;
        void         populate_command_base( RenderCommandBase* cmd_base ) const;
        static float transform_layer_to_depth( uint16_t layer );

    private:
        GPURenderer*                m_Renderer = nullptr;
        RenderContextSpecifications m_Specs    = {};

        DXSM::Color m_ClearColor = {};

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
        RenderContextCommands*       m_RenderCommandBuffer      = nullptr;    // instance owned by GPURenderer

        static uint16_t m_CurrentDepthLayer;
        static float    m_CurrentLayerDepth;
    };

    using RenderContextHandle = uint32_t;
}    // namespace InnoEngine
