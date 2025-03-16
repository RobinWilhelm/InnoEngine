#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "imgui.h"

#include "IE_Assert.h"
#include "BaseTypes.h"
#include "GPUDeviceRef.h"

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

        void set_clear_color( DXSM::Color color );    // the color the swapchain texture should be cleared to at the begin of the frame
        void set_view_projection( const DXSM::Matrix view_projection );
        void add_sprite( const Sprite& sprite );
        void add_texture( Ref<Texture2D> texture, float x, float y, uint32_t layer = 0, float rotation = 0.0f, DXSM::Color color = { 1.0f, 1.0f, 1.0f, 1.0f }, float scale = 1.0f );
        void add_text( const Font* font, float x, float y, uint32_t size, std::string_view text, DXSM::Color color = { 1.0f, 1.0f, 1.0f, 1.0f }, uint16_t layer = 0 );
        void add_imgui_draw_data( ImDrawData* draw_data );

    private:
        void retrieve_shaderformatinfo();

        // debug only
        const RenderCommandBuffer* get_render_command_buffer() const;

    private:
        bool m_initialized  = false;
        bool m_vsyncEnabled = true;

        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        GPUDeviceRef m_sdlGPUDevice = nullptr;
        Window*      m_window       = nullptr;

        bool m_doubleBuffered = false;
    };
}    // namespace InnoEngine
