#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/utility/Profiler.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/AssetRepository.h"

#include "InnoEngine/graphics/Camera.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/Sprite.h"
#include "InnoEngine/graphics/Font.h"

#include "InnoEngine/graphics/Shader.h"
#include "InnoEngine/utility/StringArena.h"

#include "RenderCommandBuffer.h"

#include "SDL3/SDL_vulkan.h"

#ifdef _DEBUG
    #define DEBUG_FRAMEBUFFERINDICES
#endif

namespace InnoEngine
{
    class GPURenderer::PipelineProcessor
    {
    public:
        Result initialize( GPURenderer* renderer, AssetManager* assetmanager )
        {
            IE_ASSERT( renderer != nullptr && assetmanager != nullptr );

            Result result = Result::Fail;

            m_Sprite2DPipeline = std::make_unique<Sprite2DPipeline>();
            result             = m_Sprite2DPipeline->initialize( renderer, assetmanager );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Failed to initialze Sprite pipeline! Errorcode: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            m_PrimitivePipeline = std::make_unique<Primitive2DPipeline>();
            result              = m_PrimitivePipeline->initialize( renderer, assetmanager );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Failed to initialze Primitives pipeline! Errorcode: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            m_Font2DPipeline = std::make_unique<Font2DPipeline>();
            result           = m_Font2DPipeline->initialize( renderer, assetmanager );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Failed to initialze Font pipeline! Errorcode: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            m_ImGuiPipeline = std::make_unique<ImGuiPipeline>();
            result          = m_ImGuiPipeline->initialize( renderer );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Failed to initialze ImGui pipeline! Errorcode: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            m_Initialized = true;
            return Result::Success;
        }

        void prepare()
        {
            IE_ASSERT( m_Initialized );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();

            m_Sprite2DPipeline->prepare_render( render_cmd_buf.SpriteRenderCommands );

            m_PrimitivePipeline->prepare_render( render_cmd_buf.QuadRenderCommands,
                                                 render_cmd_buf.LineRenderCommands,
                                                 render_cmd_buf.CircleRenderCommands );

            m_Font2DPipeline->prepare_render( render_cmd_buf.FontRenderCommands,
                                              render_cmd_buf.FontRegister,
                                              render_cmd_buf.StringBuffer );

            m_ImGuiPipeline->prepare_render( render_cmd_buf.ImGuiCommandBuffer );
        }

        void render( SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass, RenderStatistics& stats )
        {
            IE_ASSERT( m_Initialized );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();

            stats.SpriteDrawCalls = m_Sprite2DPipeline->swapchain_render( render_cmd_buf.ViewPorts,
                                                                          render_cmd_buf.TextureRegister,
                                                                          render_pass );

            stats.PrimitivesDrawCalls = m_PrimitivePipeline->swapchain_render( render_cmd_buf.ViewPorts,
                                                                               render_pass );

            stats.FontDrawCalls = m_Font2DPipeline->swapchain_render( render_cmd_buf.ViewPorts,
                                                                      render_cmd_buf.FontRegister,
                                                                      render_pass );

            stats.ImGuiDrawCalls = m_ImGuiPipeline->swapchain_render( render_cmd_buf.ImGuiCommandBuffer,
                                                                      gpu_cmd_buf,
                                                                      render_pass );
        }

        RenderCommandBuffer& get_command_buffer_for_collecting()
        {
            return m_RenderCommandBuffer.get_producer_data();
        }

        const RenderCommandBuffer& get_command_buffer_for_rendering() const
        {
            return m_RenderCommandBuffer.get_consumer_data();
        }

        void synchronize()
        {
            m_RenderCommandBuffer.swap();
        }

    private:
        DoubleBuffered<RenderCommandBuffer> m_RenderCommandBuffer;
        Own<Sprite2DPipeline>               m_Sprite2DPipeline;
        Own<Font2DPipeline>                 m_Font2DPipeline;
        Own<ImGuiPipeline>                  m_ImGuiPipeline;
        Own<Primitive2DPipeline>            m_PrimitivePipeline;

        bool m_Initialized = false;
    };

    GPURenderer::~GPURenderer()
    {
        if ( m_sdlGPUDevice ) {
            wait_for_gpu_idle();

            if ( m_CameraMatrixTransferBuffer ) {
                SDL_ReleaseGPUTransferBuffer( m_sdlGPUDevice, m_CameraMatrixTransferBuffer );
                m_CameraMatrixTransferBuffer = nullptr;
            }

            if ( m_CameraMatrixStorageBuffer ) {
                SDL_ReleaseGPUBuffer( m_sdlGPUDevice, m_CameraMatrixStorageBuffer );
                m_CameraMatrixStorageBuffer = nullptr;
            }

            if ( m_DepthTexture ) {
                SDL_ReleaseGPUTexture( m_sdlGPUDevice, m_DepthTexture );
                m_DepthTexture = nullptr;
            }

            m_pipelineProcessor.reset();

            if ( m_Window ) {
                SDL_ReleaseWindowFromGPUDevice( m_sdlGPUDevice, m_Window->get_sdlwindow() );
                m_Window = nullptr;
            }

#ifdef DEBUG_DEVICE_REF
            if ( m_sdlGPUDevice.use_count() != 1 ) {
                IE_LOG_ERROR( "Not all Deviceobjects are released!" );
            }
#endif
            SDL_DestroyGPUDevice( m_sdlGPUDevice );
            m_sdlGPUDevice = nullptr;
        }
    }

    auto GPURenderer::create() -> std::optional<Own<GPURenderer>>
    {
        Own<GPURenderer> renderer( new GPURenderer() );

        const char* driver       = nullptr;
        bool        debug_device = false;

#ifdef _DEBUG
        debug_device = true;
#endif

#ifdef DEBUG_DEVICE_REF
        renderer->m_sdlGPUDevice = GPUDeviceRef::create( SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, debug_device, driver ) );
#else
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, debug_device, driver );
#endif

        if ( renderer->m_sdlGPUDevice == nullptr ) {
            IE_LOG_CRITICAL( " SDL_CreateGPUDevice failed! Errorcode: {}", SDL_GetError() );
            return std::nullopt;
        }

        IE_LOG_DEBUG( "Selected gpu driver: {}", renderer->get_devicedriver() );

        renderer->m_pipelineProcessor = std::make_unique<PipelineProcessor>();
        renderer->retrieve_shaderformatinfo();
        return renderer;
    }

    Result GPURenderer::initialize( Window* window, AssetManager* assetmanager )
    {
        if ( m_Initialized ) {
            IE_LOG_WARNING( "GPURenderer: Trying to initialize more than once!" );
            return Result::AlreadyInitialized;
        }

        m_Window = window;
        if ( m_Window ) {
            if ( !SDL_ClaimWindowForGPUDevice( m_sdlGPUDevice, m_Window->get_sdlwindow() ) ) {
                IE_LOG_CRITICAL( "GPUClaimWindow failed! Errorcode: {}", SDL_GetError() );
                return Result::InitializationError;
            }

            SDL_GPUTextureCreateInfo depthtexture_createinfo = {};

            depthtexture_createinfo.usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            depthtexture_createinfo.format               = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
            depthtexture_createinfo.width                = m_Window->width();
            depthtexture_createinfo.height               = m_Window->height();
            depthtexture_createinfo.num_levels           = 1;
            depthtexture_createinfo.layer_count_or_depth = 1;
            depthtexture_createinfo.type                 = SDL_GPU_TEXTURETYPE_2D;

            m_DepthTexture = SDL_CreateGPUTexture( m_sdlGPUDevice, &depthtexture_createinfo );
            if ( m_DepthTexture == nullptr ) {
                IE_LOG_ERROR( "Failed to create depth texture: {}", SDL_GetError() );
                return Result::Fail;
            }

            m_FullscreenDefaultViewport = Viewport( 0, 0, static_cast<float>( m_Window->width() ), static_cast<float>( m_Window->height() ), 0.0f, 1.0f );
        }

        RETURN_RESULT_IF_FAILED( create_camera_transformation_buffers() );

        RETURN_RESULT_IF_FAILED( m_pipelineProcessor->initialize( this, assetmanager ) );
        m_Initialized = true;
        return Result::Success;
    }

    Window* GPURenderer::get_window() const
    {
        return m_Window;
    }

    GPUDeviceRef GPURenderer::get_gpudevice() const
    {
        return m_sdlGPUDevice;
    };

    bool GPURenderer::has_window()
    {
        return m_Window != nullptr;
    }

    void GPURenderer::log_available_drivers() const
    {
        IE_LOG_INFO( "Available renderer drivers:" );
        for ( int i = 0; i < SDL_GetNumRenderDrivers(); ++i ) {
            IE_LOG_INFO( "%d. %s", i + 1, SDL_GetRenderDriver( i ) );
        }
    }

    Result GPURenderer::enable_vsync( bool enabled )
    {
        if ( m_vsyncEnabled == enabled )
            return Result::Success;

        if ( enabled == false ) {
            bool supported = SDL_WindowSupportsGPUPresentMode( m_sdlGPUDevice, get_window()->get_sdlwindow(), SDL_GPU_PRESENTMODE_IMMEDIATE );
            if ( supported == false )
                return Result::Fail;
        }

        bool result = SDL_SetGPUSwapchainParameters( m_sdlGPUDevice, get_window()->get_sdlwindow(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, enabled ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE );
        if ( result ) {
            m_vsyncEnabled = enabled;
            return Result::Success;
        }
        else {
            return Result::Fail;
        }
    }

    bool GPURenderer::vsync_enabled() const
    {
        return m_vsyncEnabled;
    }

    const char* GPURenderer::get_devicedriver() const
    {
        return SDL_GetGPUDeviceDriver( m_sdlGPUDevice );
    }

    const RenderStatistics& GPURenderer::get_statistics() const
    {
        return m_Statistics.get_consumer_data();
    }

    void GPURenderer::wait_for_gpu_idle()
    {
        SDL_WaitForGPUIdle( m_sdlGPUDevice );
    }

    void GPURenderer::synchronize()
    {
#ifdef DEBUG_FRAMEBUFFERINDICES
        // reset the frame indices so they can be checked to catch errors
        auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();
        for ( auto& texture : render_commands.TextureRegister )
            texture->m_frameBufferIndex = -1;

        for ( auto& font : render_commands.FontRegister )
            font->m_frameBufferIndex = -1;
#endif
        m_pipelineProcessor->synchronize();
    }

    void GPURenderer::render()
    {
        IE_ASSERT( m_Initialized );
        const auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();

        IE_ASSERT( render_commands.ViewProjectionMatrices.size() != 0 );
        IE_ASSERT( render_commands.ViewPorts.size() != 0 );

        ProfileScoped profile_rendercommands( ProfilePoint::ProcessRenderCommands );

        // dont render when minimized
        if ( m_Window && SDL_GetWindowFlags( m_Window->get_sdlwindow() ) & SDL_WINDOW_MINIMIZED ) {
            return;
        }

        upload_camera_transformations( render_commands.ViewProjectionMatrices );

        m_pipelineProcessor->prepare();

        SDL_GPUCommandBuffer* gpu_cmd_buf = SDL_AcquireGPUCommandBuffer( m_sdlGPUDevice );
        if ( gpu_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }

        // TODO: custom rendertarget pass
        { }

        // Main swapchain pass
        // only works when a window is associated
        if ( has_window() ) {
            SDL_GPUTexture* swapchainTexture;

            {
                ProfileScoped gpu_swapchain_wait( ProfilePoint::GPUSwapChainWait );
                if ( !SDL_WaitAndAcquireGPUSwapchainTexture( gpu_cmd_buf, m_Window->get_sdlwindow(), &swapchainTexture, nullptr, nullptr ) ) {
                    SDL_CancelGPUCommandBuffer( gpu_cmd_buf );
                    IE_LOG_WARNING( "WaitAndAcquireGPUSwapchainTexture failed : %s", SDL_GetError() );
                    return;
                }
            }

            if ( swapchainTexture != nullptr ) {
                SDL_GPUDepthStencilTargetInfo depth_stencil = {};
                depth_stencil.texture                       = m_DepthTexture;
                depth_stencil.clear_depth                   = 0;
                depth_stencil.load_op                       = SDL_GPU_LOADOP_CLEAR;
                depth_stencil.store_op                      = SDL_GPU_STOREOP_STORE;

                SDL_GPUColorTargetInfo color_target = {};
                color_target.texture                = swapchainTexture;
                color_target.clear_color            = { render_commands.ClearColor.R(), render_commands.ClearColor.G(), render_commands.ClearColor.B(), render_commands.ClearColor.A() };
                color_target.load_op                = render_commands.Clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
                color_target.store_op               = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, &depth_stencil );

                SDL_BindGPUVertexStorageBuffers( render_pass, 0, &m_CameraMatrixStorageBuffer, 1 );

                m_pipelineProcessor->render( gpu_cmd_buf, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );
            }
        }

        if ( SDL_SubmitGPUCommandBuffer( gpu_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }
    }

    void GPURenderer::begin_collection()
    {
        update_statistics_from_last_completed_frame();
        auto& collect_buffer = m_pipelineProcessor->get_command_buffer_for_collecting();
        collect_buffer.clear();
        collect_buffer.ViewPorts.push_back( m_FullscreenDefaultViewport );

        use_layer( 0 );
        use_default_viewport();
        use_default_rendertarget();
    }

    void GPURenderer::end_collection()
    {
        // nothing yet but probably need to switch between multiple collecting commandbuffers in the future
    }

    void GPURenderer::register_texture( Ref<Texture2D> texture )
    {
        IE_ASSERT( texture != nullptr );

        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.TextureRegister.push_back( texture );
        texture->m_frameBufferIndex = static_cast<FrameBufferIndex>( render_cmd_buf.TextureRegister.size() - 1 );
    }

    void GPURenderer::register_sprite( Sprite& sprite )
    {
        IE_ASSERT( sprite.m_texture != nullptr );

        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.TextureRegister.push_back( sprite.m_texture );
        sprite.m_texture->m_frameBufferIndex = static_cast<FrameBufferIndex>( render_cmd_buf.TextureRegister.size() - 1 );
    }

    void GPURenderer::register_font( Ref<Font> font )
    {
        IE_ASSERT( font != nullptr && font->m_Initialized );

        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.FontRegister.push_back( font );
        font->m_frameBufferIndex = static_cast<FrameBufferIndex>( render_cmd_buf.FontRegister.size() - 1 );
    }

    void GPURenderer::set_clear_color( DXSM::Color color )
    {
        auto& render_cmd_buf      = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.Clear      = true;
        render_cmd_buf.ClearColor = color;
    }

    CameraIndexType GPURenderer::use_camera( const Ref<Camera> camera )
    {
        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        IE_ASSERT( render_cmd_buf.ViewProjectionMatrices.size() <= ( std::numeric_limits<uint8_t>::max )() );
        m_CurrentViewProjectionIndex = static_cast<uint8_t>( render_cmd_buf.ViewProjectionMatrices.size() );
        render_cmd_buf.ViewProjectionMatrices.push_back( camera->get_viewprojectionmatrix() );
        return m_CurrentViewProjectionIndex;
    }

    CameraIndexType GPURenderer::use_camera( const Camera* camera )
    {
        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        IE_ASSERT( render_cmd_buf.ViewProjectionMatrices.size() <= ( std::numeric_limits<uint8_t>::max )() );
        m_CurrentViewProjectionIndex = static_cast<uint8_t>( render_cmd_buf.ViewProjectionMatrices.size() );
        render_cmd_buf.ViewProjectionMatrices.push_back( camera->get_viewprojectionmatrix() );
        return m_CurrentViewProjectionIndex;
    }

    void GPURenderer::use_camera_by_index( CameraIndexType camera_index )
    {
        IE_ASSERT( camera_index < m_pipelineProcessor->get_command_buffer_for_collecting().ViewProjectionMatrices.size() );
        m_CurrentViewProjectionIndex = camera_index;
    }

    void GPURenderer::use_default_viewport()
    {
        IE_ASSERT( m_pipelineProcessor->get_command_buffer_for_collecting().ViewPorts.size() > 0 );
        m_CurrentViewPortIndex = 0;
    }

    const Viewport& GPURenderer::get_default_viewport() const
    {
        return m_FullscreenDefaultViewport;
    }

    void GPURenderer::use_default_rendertarget()
    {
        IE_ASSERT( m_Window != nullptr );
        m_CurrentRenderTargetIndex = 0;
    }

    uint8_t GPURenderer::use_view_port( const Viewport& view_port )
    {
        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        IE_ASSERT( render_cmd_buf.ViewPorts.size() <= ( std::numeric_limits<uint8_t>::max )() );
        m_CurrentViewPortIndex = static_cast<uint8_t>( render_cmd_buf.ViewPorts.size() );
        render_cmd_buf.ViewPorts.push_back( view_port );
        return m_CurrentViewPortIndex;
    }

    void GPURenderer::use_view_port_index( uint8_t view_port_index )
    {
        IE_ASSERT( view_port_index < m_pipelineProcessor->get_command_buffer_for_collecting().ViewPorts.size() );
        m_CurrentViewPortIndex = view_port_index;
    }

    void GPURenderer::add_sprite( const Sprite& sprite )
    {
        IE_ASSERT( sprite.m_texture != nullptr && sprite.m_texture->m_frameBufferIndex >= 0 );

        RenderCommandBuffer&       render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Sprite2DPipeline::Command& cmd            = render_cmd_buf.SpriteRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.TextureIndex = sprite.m_texture->m_frameBufferIndex;
        cmd.Position     = sprite.m_position;
        cmd.Size         = { sprite.m_scale.x * sprite.m_texture->m_width, sprite.m_scale.y * sprite.m_texture->m_height };
        cmd.OriginOffset = sprite.m_originOffset * cmd.Size;
        cmd.SourceRect   = sprite.m_sourceRect;
        cmd.Color        = sprite.m_color;
        cmd.Rotation     = DirectX::XMConvertToRadians( sprite.m_rotationDegrees );
    }

    void GPURenderer::add_pixel( const DXSM::Vector2& position, const DXSM::Color& color )
    {
        add_quad( position,
                  { 1.0f, 1.0f },
                  0.0f,
                  color );
    }

    void GPURenderer::add_quad( const DXSM::Vector2& position, const DXSM::Vector2& size, float rotation, const DXSM::Color& color )
    {
        RenderCommandBuffer&              render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Primitive2DPipeline::QuadCommand& cmd            = render_cmd_buf.QuadRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.Position = position;
        cmd.Size     = size;
        cmd.Rotation = DirectX::XMConvertToRadians( rotation );
        cmd.Color    = color;
    }

    void GPURenderer::add_line( const DXSM::Vector2& start, const DXSM::Vector2& end, float thickness, float edge_fade, const DXSM::Color& color )
    {
        RenderCommandBuffer&              render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Primitive2DPipeline::LineCommand& cmd            = render_cmd_buf.LineRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.Start     = start;
        cmd.End       = end;
        cmd.Color     = color;
        cmd.Thickness = thickness;
        cmd.EdgeFade  = edge_fade;
    }

    void GPURenderer::add_lines( const std::vector<DXSM::Vector2>& points, float thickness, float edge_fade, const DXSM::Color& color, bool loop )
    {
        uint32_t point_amount = static_cast<uint32_t>( points.size() );
        if ( point_amount == 1 )
            return;

        RenderCommandBuffer& render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();

        for ( size_t i = 0; i < point_amount; ++i ) {
            if ( i + 1 < point_amount ) {
                Primitive2DPipeline::LineCommand& cmd = render_cmd_buf.LineRenderCommands.emplace_back();
                populate_command_base( &cmd );
                cmd.Start     = points[ i ];
                cmd.End       = points[ i + 1 ];
                cmd.Color     = color;
                cmd.Thickness = thickness;
                cmd.EdgeFade  = edge_fade;
            }
            else if ( loop ) {
                Primitive2DPipeline::LineCommand& cmd = render_cmd_buf.LineRenderCommands.emplace_back();
                populate_command_base( &cmd );
                cmd.Start     = points[ i ];
                cmd.End       = points[ 0 ];
                cmd.Color     = color;
                cmd.Thickness = thickness;
                cmd.EdgeFade  = edge_fade;
            }
        }
    }

    void GPURenderer::add_circle( const DXSM::Vector2& position, float radius, float edge_fade, const DXSM::Color& color )
    {
        add_circle( position, radius, radius, edge_fade, color );
    }

    void GPURenderer::add_circle( const DXSM::Vector2& position, float radius, float thickness, float edge_fade, const DXSM::Color& color )
    {
        RenderCommandBuffer&                render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Primitive2DPipeline::CircleCommand& cmd            = render_cmd_buf.CircleRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.Position.x = position.x - radius;
        cmd.Position.y = position.y - radius;
        cmd.Color      = color;
        cmd.Radius     = radius;
        cmd.Fade       = edge_fade;
        cmd.Thickness  = thickness / radius;
    }

    void GPURenderer::add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position )
    {
        add_textured_quad( texture,
                           position,
                           { 1.0f, 1.0f },
                           0.0f,
                           { 1.0f, 1.0f, 1.0f, 1.0f } );
    }

    void GPURenderer::add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color )
    {
        add_textured_quad( texture,
                           { 0.0f, 0.0f, 1.0f, 1.0f },
                           position,
                           scale,
                           rotation,
                           color );
    }

    void GPURenderer::add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position )
    {
        add_textured_quad( texture,
                           source_rect,
                           position,
                           { 1.0f, 1.0f },
                           0.0f,
                           { 1.0f, 1.0f, 1.0f, 1.0f } );
    }

    void GPURenderer::add_textured_quad( Ref<Texture2D> texture, const DXSM::Vector4& source_rect, const DXSM::Vector2& position, const DXSM::Vector2& scale, float rotation, const DXSM::Color& color )
    {
        IE_ASSERT( texture != nullptr && texture->m_frameBufferIndex >= 0 );

        RenderCommandBuffer&       render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Sprite2DPipeline::Command& cmd            = render_cmd_buf.SpriteRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.TextureIndex = texture->m_frameBufferIndex;
        cmd.Position     = position;
        cmd.Size         = { scale.x * texture->m_width, scale.y * texture->m_height };
        cmd.OriginOffset = cmd.Size * 0.5f;
        cmd.SourceRect   = source_rect;
        cmd.Color        = color;
        cmd.Rotation     = DirectX::XMConvertToRadians( rotation );
    }

    void GPURenderer::add_imgui_draw_data( ImDrawData* draw_data )
    {
        IE_ASSERT( draw_data != nullptr );

        RenderCommandBuffer&        render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        ImGuiPipeline::CommandData& cmd            = render_cmd_buf.ImGuiCommandBuffer;

        cmd.FrameBufferScale = draw_data->FramebufferScale;
        cmd.TotalIndexCount  = draw_data->TotalIdxCount;
        cmd.TotalVertexCount = draw_data->TotalVtxCount;
        cmd.DisplayPos       = draw_data->DisplayPos;
        cmd.DisplaySize      = draw_data->DisplaySize;

        cmd.RenderCommandLists.reserve( draw_data->CmdListsCount );
        for ( int n = 0; n < draw_data->CmdListsCount; n++ ) {
            const ImDrawList* drawList   = draw_data->CmdLists[ n ];
            auto&             renderList = cmd.RenderCommandLists.emplace_back();
            renderList.CommandBuffer     = drawList->CmdBuffer;
            renderList.VertexBuffer      = drawList->VtxBuffer;
            renderList.IndexBuffer       = drawList->IdxBuffer;
        }
    }

    void GPURenderer::add_bounding_box( const DXSM::Vector4& aabb, const DXSM::Vector2& position, const DXSM::Color& color )
    {
        float line_width = 2;

        std::vector<DXSM::Vector2> points {
            { aabb.x + position.x - line_width, aabb.y + position.y + line_width },
            { aabb.z + line_width + position.x, aabb.y + position.y + line_width },
            { aabb.z + line_width + position.x, aabb.w - line_width + position.y },
            { aabb.x + position.x - line_width, aabb.w - line_width + position.y },
        };
        add_lines( points, line_width, 0.0f, color, true );
    }

    uint16_t GPURenderer::use_next_layer()
    {
        m_CurrentLayerDepth = transform_layer_to_depth( ++m_CurrentLayer );
        return m_CurrentLayer;
    }

    void GPURenderer::use_layer( uint16_t layer )
    {
        m_CurrentLayer      = layer;
        m_CurrentLayerDepth = transform_layer_to_depth( m_CurrentLayer );
    }

    float GPURenderer::transform_layer_to_depth( uint16_t layer )
    {
        return layer == 0 ? 0.0f : static_cast<float>( layer ) / 65536.0f;
    }

    void GPURenderer::update_statistics_from_last_completed_frame()
    {
        const auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();
        auto&       stats           = m_Statistics.get_producer_data();

        stats.TotalBufferSize += sizeof( DXSM::Color );
        stats.TotalBufferSize += sizeof( DXSM::Matrix );
        stats.TotalBufferSize += render_commands.TextureRegister.size() * sizeof( Ref<Texture2D> );

        stats.TotalCommands += render_commands.SpriteRenderCommands.size();
        stats.TotalBufferSize += render_commands.SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command );

        stats.TotalCommands += render_commands.QuadRenderCommands.size();
        stats.TotalBufferSize += render_commands.QuadRenderCommands.size() * sizeof( Primitive2DPipeline::QuadCommand );

        stats.TotalCommands += render_commands.LineRenderCommands.size();
        stats.TotalBufferSize += render_commands.LineRenderCommands.size() * sizeof( Primitive2DPipeline::LineCommand );

        stats.TotalCommands += render_commands.CircleRenderCommands.size();
        stats.TotalBufferSize += render_commands.CircleRenderCommands.size() * sizeof( Primitive2DPipeline::CircleCommand );

        stats.TotalBufferSize += render_commands.FontRegister.size() * sizeof( Ref<Font> );
        stats.TotalCommands += render_commands.FontRenderCommands.size();
        stats.TotalBufferSize += render_commands.FontRenderCommands.size() * sizeof( Font2DPipeline::Command );
        stats.TotalBufferSize += render_commands.StringBuffer.size();

        for ( const auto& rcmd : render_commands.ImGuiCommandBuffer.RenderCommandLists ) {
            stats.TotalCommands += rcmd.CommandBuffer.size();
            stats.TotalBufferSize += rcmd.CommandBuffer.size() * sizeof( ImDrawCmd );
            stats.TotalBufferSize += rcmd.IndexBuffer.size() * sizeof( ImDrawIdx );
            stats.TotalBufferSize += rcmd.VertexBuffer.size() * sizeof( ImDrawVert );
        }

        stats.TotalDrawCalls = stats.SpriteDrawCalls + stats.FontDrawCalls + stats.ImGuiDrawCalls;

        m_Statistics.swap();
        m_Statistics.get_producer_data() = RenderStatistics();
    }

    const RenderCommandBuffer* GPURenderer::get_render_command_buffer() const
    {
        return &m_pipelineProcessor->get_command_buffer_for_collecting();
    }

    void GPURenderer::populate_command_base( RenderCommandBase* command )
    {
        command->Depth             = m_CurrentLayerDepth;
        command->CameraIndex       = m_CurrentViewProjectionIndex;
        command->RenderTargetIndex = m_CurrentRenderTargetIndex;
    }

    Result GPURenderer::create_camera_transformation_buffers()
    {
        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = 256 * sizeof( DXSM::Matrix );

        m_CameraMatrixTransferBuffer = SDL_CreateGPUTransferBuffer( m_sdlGPUDevice, &tbufferCreateInfo );
        if ( m_CameraMatrixTransferBuffer == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUTransferBuffer! {}", SDL_GetError() );
            return Result::Fail;
        }

        SDL_GPUBufferCreateInfo createInfo = {};
        createInfo.usage                   = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        createInfo.size                    = 256 * sizeof( DXSM::Matrix );

        m_CameraMatrixStorageBuffer = SDL_CreateGPUBuffer( m_sdlGPUDevice, &createInfo );
        if ( m_CameraMatrixStorageBuffer == nullptr ) {
            // TODO: this needs to be handled better
            IE_LOG_ERROR( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() );
            return Result::Fail;
        }
        return Result::Success;
    }

    void GPURenderer::upload_camera_transformations( const std::vector<DXSM::Matrix>& camera_transforms )
    {
        IE_ASSERT( m_sdlGPUDevice != nullptr );
        IE_ASSERT( m_CameraMatrixTransferBuffer != nullptr );
        IE_ASSERT( m_CameraMatrixStorageBuffer != nullptr );

        DXSM::Matrix* buffer_data = static_cast<DXSM::Matrix*>( SDL_MapGPUTransferBuffer( m_sdlGPUDevice, m_CameraMatrixTransferBuffer, true ) );
        std::memcpy( buffer_data, camera_transforms.data(), sizeof( DXSM::Matrix ) * camera_transforms.size() );

        SDL_UnmapGPUTransferBuffer( m_sdlGPUDevice, m_CameraMatrixTransferBuffer );
        SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_CameraMatrixTransferBuffer, .offset = 0 };
        SDL_GPUBufferRegion           bufferRegion { .buffer = m_CameraMatrixStorageBuffer,
                                                     .offset = 0,
                                                     .size   = static_cast<uint32_t>( camera_transforms.size() * sizeof( DXSM::Matrix ) ) };

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_sdlGPUDevice );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );
        SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );
        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    void GPURenderer::add_text( const Font* font, const DXSM::Vector2& position, uint32_t size, std::string_view text, const DXSM::Color& color )
    {
        IE_ASSERT( font != nullptr && font->m_frameBufferIndex >= 0 );

        RenderCommandBuffer&     render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Font2DPipeline::Command& cmd            = render_cmd_buf.FontRenderCommands.emplace_back();

        populate_command_base( &cmd );
        cmd.FontFBIndex     = font->m_frameBufferIndex;
        cmd.StringIndex     = render_cmd_buf.StringBuffer.insert( text );
        cmd.StringLength    = static_cast<uint32_t>( text.size() );
        cmd.Position        = position;
        cmd.FontSize        = size;
        cmd.ForegroundColor = color;
    }

    void GPURenderer::add_text_centered( const Font* font, const DXSM::Vector2& position, uint32_t text_size, std::string_view text, const DXSM::Color& color )
    {
        DXSM::Vector4 aabb        = font->get_aabb( text_size, text );
        float         text_width  = aabb.z - aabb.x;
        float         text_height = aabb.y - aabb.w;
        add_text( font, { position.x - text_width / 2, position.y - text_height / 2 }, text_size, text, color );
    }

    void GPURenderer::retrieve_shaderformatinfo()
    {
        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats( m_sdlGPUDevice );
        auto&               shaderFormat   = Shader::ms_shaderFormat;

        if ( backendFormats & SDL_GPU_SHADERFORMAT_SPIRV ) {
            shaderFormat.SubDirectory      = "SPIRV";
            shaderFormat.Format            = SDL_GPU_SHADERFORMAT_SPIRV;
            shaderFormat.EntryPoint        = "main";
            shaderFormat.FileNameExtension = ".spv";
        }
        else if ( backendFormats & SDL_GPU_SHADERFORMAT_MSL ) {
            shaderFormat.SubDirectory      = "MSL";
            shaderFormat.Format            = SDL_GPU_SHADERFORMAT_MSL;
            shaderFormat.EntryPoint        = "main0";
            shaderFormat.FileNameExtension = ".msl";
        }
        else if ( backendFormats & SDL_GPU_SHADERFORMAT_DXIL ) {
            shaderFormat.SubDirectory      = "DXIL";
            shaderFormat.Format            = SDL_GPU_SHADERFORMAT_DXIL;
            shaderFormat.EntryPoint        = "main";
            shaderFormat.FileNameExtension = ".dxil";
        }
    }
}    // namespace InnoEngine
