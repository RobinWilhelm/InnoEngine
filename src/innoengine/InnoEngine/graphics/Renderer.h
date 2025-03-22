#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "imgui.h"

#include "InnoEngine/IE_Assert.h"
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/Camera.h"

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
    class RenderContext;

    struct RenderStatistics
    {
        size_t SpriteDrawCalls     = 0;
        size_t PrimitivesDrawCalls = 0;
        size_t FontDrawCalls       = 0;
        size_t ImGuiDrawCalls      = 0;

        size_t TotalCommands   = 0;
        size_t TotalDrawCalls  = 0;
        size_t TotalBufferSize = 0;
    };

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

        const RenderStatistics& get_statistics() const;    // get the stats with of the last completed fram

        void wait_for_gpu_idle();
        void synchronize();

        void render();    // process all available rendercommands and send them to the gpu
        void begin_collection();
        void end_collection();

        // Important: needs to be externally synchronized when adding rendercommands from multiple threads
        // currently the commands are only synchronized between update and main thread once per frame
        void register_texture( Ref<Texture2D> texture );
        void register_sprite( Sprite& sprite );
        void register_font( Ref<Font> font );

        void set_clear_color( DXSM::Color color );    // the color the swapchain texture should be cleared to at the begin of the frame
        void add_bounding_box( const DXSM::Vector4& aabb, const DXSM::Vector2& position, const DXSM::Color& color );

        void bind_rendercontext( Ref<RenderContext> render_ctx );

    private:
        void  retrieve_shaderformatinfo();;

        void update_statistics_from_last_completed_frame();

        // debug only
        RenderCommandBuffer* get_render_command_buffer() const;

        Result create_camera_transformation_buffers();
        void   upload_camera_transformations( const std::vector<Ref<RenderContext>>& registered_rendercontexts );

    private:
        bool m_Initialized  = false;
        bool m_vsyncEnabled = true;

        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        GPUDeviceRef m_sdlGPUDevice = nullptr;
        Window*      m_Window       = nullptr;

        SDL_GPUTexture* m_DepthTexture = nullptr;

        DoubleBuffered<RenderStatistics> m_Statistics;

        SDL_GPUTransferBuffer* m_CameraMatrixTransferBuffer = nullptr;
        SDL_GPUBuffer*         m_CameraMatrixStorageBuffer  = nullptr;
    };
}    // namespace InnoEngine
