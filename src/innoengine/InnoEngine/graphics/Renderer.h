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

        uint16_t use_next_layer();
        void     use_layer( uint16_t layer );

        CameraIndexType use_camera( const Ref<Camera> camera );
        CameraIndexType use_camera( const Camera* camera );
        void            use_camera_by_index( CameraIndexType camera_index );

        void            use_default_viewport();
        const Viewport& get_default_viewport() const;

        void use_default_rendertarget();

        uint8_t use_view_port( const Viewport& view_port );
        void    use_view_port_index( uint8_t view_port_index );

        void set_clear_color( DXSM::Color color );    // the color the swapchain texture should be cleared to at the begin of the frame

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

        void add_text( const Font* font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color );
        void add_text_centered( const Font* font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color );    // recalculates text size every time
        void add_imgui_draw_data( ImDrawData* draw_data );

        void add_bounding_box( const DXSM::Vector4& aabb, const DXSM::Vector2& position, const DXSM::Color& color );

    private:
        void  retrieve_shaderformatinfo();
        float transform_layer_to_depth( uint16_t layer );

        void update_statistics_from_last_completed_frame();

        // debug only
        const RenderCommandBuffer* get_render_command_buffer() const;

        void populate_command_base( RenderCommandBase* command );

        Result create_camera_transformation_buffers();
        void   upload_camera_transformations( const std::vector<DXSM::Matrix>& camera_transforms );

    private:
        bool m_Initialized  = false;
        bool m_vsyncEnabled = true;

        uint16_t m_CurrentLayer      = 0;
        float    m_CurrentLayerDepth = 0.0f;

        CameraIndexType       m_CurrentViewProjectionIndex = 0;
        ViewPortIndexType     m_CurrentViewPortIndex       = 0;
        RenderTargetIndexType m_CurrentRenderTargetIndex   = 0;

        class PipelineProcessor;
        Own<PipelineProcessor> m_pipelineProcessor = nullptr;

        GPUDeviceRef m_sdlGPUDevice = nullptr;
        Window*      m_Window       = nullptr;

        SDL_GPUTexture* m_DepthTexture = nullptr;

        DoubleBuffered<RenderStatistics> m_Statistics;
        Viewport                         m_FullscreenDefaultViewport;

        SDL_GPUTransferBuffer* m_CameraMatrixTransferBuffer = nullptr;
        SDL_GPUBuffer*         m_CameraMatrixStorageBuffer  = nullptr;
    };
}    // namespace InnoEngine
