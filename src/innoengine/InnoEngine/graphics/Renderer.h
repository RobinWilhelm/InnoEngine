#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "imgui.h"

#include "InnoEngine/IE_Assert.h"
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

#include <string>
#include <atomic>
#include <optional>

namespace InnoEngine
{
    class Window;
    class OrthographicCamera;
    class Sprite;
    class AssetManager;
    class Font;
    class Texture2D;
    struct RenderCommandBuffer;

    class GPURenderer
    {
        friend class DebugUI;
        GPURenderer() = default;

    public:
        ~GPURenderer();

        [[nodiscard]]
        static auto create() -> std::optional<Own<GPURenderer>>;
        Result      initialize( Window* window, AssetManager* assetmanager );

        Window*      get_window() const;
        GPUDeviceRef get_gpudevice() const;
        bool         has_window();

        void log_available_drivers() const;

        Result enable_vsync( bool enabled );
        bool   vsync_enabled() const;

        const char* get_devicedriver() const;

        void wait_for_gpu_idle();
        void on_synchronize();
        void render();    // process all available rendercommands and send them to the gpu

        // Important: needs to be externally synchronized when adding rendercommands from multiple threads
        // currently the commands are only synchronized between update and main thread once per frame
        void register_texture( Ref<Texture2D> texture );
        void register_sprite( Sprite& sprite );
        void register_font( Ref<Font> font );

        uint16_t next_layer();
        void     set_layer( uint16_t layer );

        void set_clear_color( DXSM::Color color );    // the color the swapchain texture should be cleared to at the begin of the frame
        void set_view_projection( const DXSM::Matrix view_projection );

        void add_sprite( const Sprite& sprite );
        void add_pixel( const DXSM::Vector2& position, const DXSM::Color& color );
        void add_quad( const DXSM::Vector2& position, const DXSM::Vector2& size, float rotation, const DXSM::Color& color );
        void add_line( const DXSM::Vector2& start, const DXSM::Vector2& end, float thickness, float edge_fade, const DXSM::Color& color );
        void add_lines( const std::vector<DXSM::Vector2>& points, float thickness, float edge_fade, const DXSM::Color& color, bool loop );            
        void add_circle(const DXSM::Vector2& position, float radius, float edge_fade, const DXSM::Color& color);
        void add_circle(const DXSM::Vector2& position, float radius, float thickness, float edge_fade, const DXSM::Color& color);

        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position );
        void add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color );

        void add_text( const Font* font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, DXSM::Color color );
        void add_imgui_draw_data( ImDrawData* draw_data );

    private:
        void  retrieve_shaderformatinfo();
        float transform_layer_to_depth( uint16_t layer );

        // debug only
        const RenderCommandBuffer* get_render_command_buffer() const;

    private:
        bool m_Initialized  = false;
        bool m_vsyncEnabled = true;

        uint16_t m_CurrentLayer      = 0;
        float    m_CurrentLayerDepth = 0.0f;

        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        GPUDeviceRef m_sdlGPUDevice = nullptr;
        Window*      m_window       = nullptr;

        SDL_GPUTexture* m_DepthTexture = nullptr;

        bool m_doubleBuffered = false;
    };
}    // namespace InnoEngine
