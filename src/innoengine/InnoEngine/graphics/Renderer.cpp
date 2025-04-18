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

        uint32_t prepare_opaque( const RenderContextFrameData& render_ctx_data )
        {
            IE_ASSERT( m_Initialized );
            IE_ASSERT( render_ctx_data.Index != InvalidRenderCommandBufferIndex );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();

            uint32_t batch_count = 0;
            batch_count += m_Sprite2DPipeline->prepare_render_opaque( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].SpriteRenderCommandsOpaque );

            batch_count += m_PrimitivePipeline->prepare_render_opaque( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].QuadRenderCommandsOpaque,
                                                                       render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].LineRenderCommands,
                                                                       render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].CircleRenderCommands );

            batch_count += m_Font2DPipeline->prepare_render_opaque( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].FontRenderCommands,
                                                                    render_cmd_buf.FontRegister,
                                                                    render_cmd_buf.StringBuffer );
            return batch_count;
        }

        uint32_t prepare( const RenderContextFrameData& render_ctx_data )
        {
            IE_ASSERT( m_Initialized );
            IE_ASSERT( render_ctx_data.Index != InvalidRenderCommandBufferIndex );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();

            uint32_t batch_count = 0;
            batch_count += m_Sprite2DPipeline->prepare_render( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].SpriteRenderCommands );

            batch_count += m_PrimitivePipeline->prepare_render( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].QuadRenderCommands,
                                                                render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].LineRenderCommands,
                                                                render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].CircleRenderCommands );

            batch_count += m_Font2DPipeline->prepare_render( render_cmd_buf.RenderContextCommands[ render_ctx_data.Index ].FontRenderCommands,
                                                             render_cmd_buf.FontRegister,
                                                             render_cmd_buf.StringBuffer );
            return batch_count;
        }

        void render( const RenderContextFrameData& render_ctx_data, SDL_GPURenderPass* render_pass, RenderStatistics& stats )
        {
            IE_ASSERT( m_Initialized );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();

            stats.SpriteDrawCalls += m_Sprite2DPipeline->swapchain_render( render_ctx_data,
                                                                           render_cmd_buf.TextureRegister,
                                                                           render_pass );

            stats.PrimitivesDrawCalls += m_PrimitivePipeline->swapchain_render( render_ctx_data,
                                                                                render_pass );

            stats.FontDrawCalls += m_Font2DPipeline->swapchain_render( render_ctx_data,
                                                                       render_cmd_buf.FontRegister,
                                                                       render_pass );
        }

        uint32_t prepare_imgui()
        {
            IE_ASSERT( m_Initialized );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();
            return m_ImGuiPipeline->prepare_render( render_cmd_buf.ImGuiCommandBuffer );
        }

        void render_imgui( SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass, RenderStatistics& stats )
        {
            IE_ASSERT( m_Initialized );
            const RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();
            stats.ImGuiDrawCalls += m_ImGuiPipeline->swapchain_render( render_cmd_buf.ImGuiCommandBuffer,
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

            m_RenderContextCache.clear();
            m_RenderContextRegisterQueue.clear();

            if ( m_DebugFont != nullptr ) {
                m_DebugFont.reset();
            }

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
        renderer->m_RenderContextCache.reserve( 256 );

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
            depthtexture_createinfo.width                = m_Window->get_width();
            depthtexture_createinfo.height               = m_Window->get_height();
            depthtexture_createinfo.num_levels           = 1;
            depthtexture_createinfo.layer_count_or_depth = 1;
            depthtexture_createinfo.type                 = SDL_GPU_TEXTURETYPE_2D;

            m_DepthTexture = SDL_CreateGPUTexture( m_sdlGPUDevice, &depthtexture_createinfo );
            if ( m_DepthTexture == nullptr ) {
                IE_LOG_ERROR( "Failed to create depth texture: {}", SDL_GetError() );
                return Result::Fail;
            }
        }

        RETURN_RESULT_IF_FAILED( create_camera_transformation_buffers() );

        RETURN_RESULT_IF_FAILED( m_pipelineProcessor->initialize( this, assetmanager ) );

        if ( auto fontOpt = CoreAPI::get_assetmanager()->require_asset<Font>( "Calibri.ttf", true ) ) {
            m_DebugFont = fontOpt.value().get();
        }

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

        bool result = SDL_SetGPUSwapchainParameters( m_sdlGPUDevice, get_window()->get_sdlwindow(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, enabled ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE );
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
        auto& collect_buffer = m_pipelineProcessor->get_command_buffer_for_collecting();
        for ( auto& texture : collect_buffer.TextureRegister ) {
            texture->m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
        }

        for ( auto& font : collect_buffer.FontRegister ) {
            font->m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
        }

        m_pipelineProcessor->synchronize();
        // now we can add the new render contexts
        std::unique_lock<std::mutex> ulock( m_RenderContextRegisterMutex );
        for ( auto specs : m_RenderContextRegisterQueue ) {
            m_RenderContextCache.emplace_back( RenderContext::create( this, specs ) );
        }
        m_RenderContextRegisterQueue.clear();
        IE_ASSERT( m_RenderContextCache.size() <= 256 );    // need to make camera storage buffer resizable if there are more than 256 cameras
    }

    void GPURenderer::render()
    {
        ProfileScoped profile_rendercommands( ProfilePoint::ProcessRenderCommands );

        IE_ASSERT( m_Initialized );
        const RenderCommandBuffer& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();

        IE_ASSERT( m_RenderContextCache.size() <= 256 );

        // dont render when minimized or when no render data is available
        if ( m_RenderContextCache.size() == 0 ||
             ( m_Window && SDL_GetWindowFlags( m_Window->get_sdlwindow() ) & SDL_WINDOW_MINIMIZED ) ) {
            return;
        }

        upload_camera_transformations( render_commands.RenderContextData );

        SDL_GPUCommandBuffer* gpu_cmd_buf = SDL_AcquireGPUCommandBuffer( m_sdlGPUDevice );
        if ( gpu_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }

        render_custom_passes( gpu_cmd_buf, render_commands );

        // main swapchain pass
        if ( has_window() ) {    // only works when a window is associated
            render_swapchain_passes( gpu_cmd_buf, render_commands );
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

        for ( auto& render_ctx : m_RenderContextCache ) {
            render_ctx->m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
            render_ctx->m_RenderCommandBuffer      = nullptr;
        }

        RenderContext::use_specific_depth_layer( 0 );
    }

    void GPURenderer::end_collection()
    {
        // nothing yet but probably need to switch between multiple collecting commandbuffers in the future
    }

    RenderContextHandle GPURenderer::create_rendercontext( RenderContextSpecifications specs )
    {
        std::unique_lock<std::mutex> ulock( m_RenderContextRegisterMutex );
        size_t                       queued_amount = m_RenderContextRegisterQueue.size();
        m_RenderContextRegisterQueue.push_back( specs );
        return static_cast<RenderContextHandle>( m_RenderContextCache.size() + queued_amount );
    }

    RenderContext* GPURenderer::acquire_rendercontext( RenderContextHandle handle )
    {
        if ( handle >= m_RenderContextCache.size() )
            return nullptr;

        auto&                        cmd_buffer = m_pipelineProcessor->get_command_buffer_for_collecting();
        RenderCommandBufferIndexType index      = static_cast<RenderCommandBufferIndexType>( cmd_buffer.RenderContextData.size() );

        Ref<RenderContext> render_ctx          = m_RenderContextCache[ handle ];
        render_ctx->m_RenderCommandBufferIndex = index;
        render_ctx->m_RenderCommandBuffer      = &cmd_buffer.RenderContextCommands[ index ];

        auto&       render_ctx_data          = cmd_buffer.RenderContextData.emplace_back();
        const auto& vp                       = render_ctx->get_viewport();
        render_ctx_data.Viewport             = { vp.LeftOffset, vp.TopOffset, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth };
        render_ctx_data.RenderTarget         = render_ctx->m_Specs.ColorTarget;
        render_ctx_data.ClearColor           = render_ctx->m_ClearColor;
        render_ctx_data.ViewProjectionMatrix = render_ctx->get_camera()->get_viewprojectionmatrix();
        render_ctx_data.Index                = index;

        return render_ctx.get();
    }

    void GPURenderer::set_clear_color( DXSM::Color color )
    {
        auto& render_cmd_buf      = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.Clear      = true;
        render_cmd_buf.ClearColor = color;
    }

    void GPURenderer::add_imgui_draw_data( ImDrawData* draw_data )
    {
        IE_ASSERT( draw_data != nullptr );

        ImGuiPipeline::CommandData& cmd = m_pipelineProcessor->get_command_buffer_for_collecting().ImGuiCommandBuffer;
        cmd.FrameBufferScale            = draw_data->FramebufferScale;
        cmd.TotalIndexCount             = draw_data->TotalIdxCount;
        cmd.TotalVertexCount            = draw_data->TotalVtxCount;
        cmd.DisplayPos                  = draw_data->DisplayPos;
        cmd.DisplaySize                 = draw_data->DisplaySize;

        cmd.RenderCommandLists.reserve( draw_data->CmdListsCount );
        for ( int n = 0; n < draw_data->CmdListsCount; n++ ) {
            const ImDrawList* drawList   = draw_data->CmdLists[ n ];
            auto&             renderList = cmd.RenderCommandLists.emplace_back();
            renderList.CommandBuffer     = drawList->CmdBuffer;
            renderList.VertexBuffer      = drawList->VtxBuffer;
            renderList.IndexBuffer       = drawList->IdxBuffer;
        }
    }

    Ref<Font> GPURenderer::get_debug_font() const
    {
        return m_DebugFont;
    }

    /*
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
    */
    void GPURenderer::update_statistics_from_last_completed_frame()
    {
        const auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();
        auto&       stats           = m_Statistics.get_producer_data();

        stats.TotalBufferSize += sizeof( DXSM::Color );
        stats.TotalBufferSize += sizeof( DXSM::Matrix );
        stats.TotalBufferSize += render_commands.TextureRegister.size() * sizeof( Ref<Texture2D> );

        for ( const auto& ctx_cmd : render_commands.RenderContextCommands ) {

            stats.TotalCommands += ctx_cmd.SpriteRenderCommands.size();
            stats.TotalBufferSize += ctx_cmd.SpriteRenderCommands.size() * sizeof( Sprite2DPipeline::Command );

            stats.TotalCommands += ctx_cmd.QuadRenderCommands.size();
            stats.TotalBufferSize += ctx_cmd.QuadRenderCommands.size() * sizeof( Primitive2DPipeline::QuadCommand );

            stats.TotalCommands += ctx_cmd.LineRenderCommands.size();
            stats.TotalBufferSize += ctx_cmd.LineRenderCommands.size() * sizeof( Primitive2DPipeline::LineCommand );

            stats.TotalCommands += ctx_cmd.CircleRenderCommands.size();
            stats.TotalBufferSize += ctx_cmd.CircleRenderCommands.size() * sizeof( Primitive2DPipeline::CircleCommand );

            stats.TotalCommands += ctx_cmd.FontRenderCommands.size();
            stats.TotalBufferSize += ctx_cmd.FontRenderCommands.size() * sizeof( Font2DPipeline::Command );
        }

        stats.TotalBufferSize += render_commands.FontRegister.size() * sizeof( Ref<Font> );
        stats.TotalBufferSize += render_commands.StringBuffer.size();

        for ( const auto& rcmd : render_commands.ImGuiCommandBuffer.RenderCommandLists ) {
            stats.TotalCommands += rcmd.CommandBuffer.size();
            stats.TotalBufferSize += rcmd.CommandBuffer.size() * sizeof( ImDrawCmd );
            stats.TotalBufferSize += rcmd.IndexBuffer.size() * sizeof( ImDrawIdx );
            stats.TotalBufferSize += rcmd.VertexBuffer.size() * sizeof( ImDrawVert );
        }

        stats.TotalDrawCalls = stats.SpriteDrawCalls + stats.FontDrawCalls + stats.ImGuiDrawCalls + stats.PrimitivesDrawCalls;

        m_Statistics.swap();
        m_Statistics.get_producer_data() = RenderStatistics();
    }

    RenderCommandBuffer* GPURenderer::get_render_command_buffer() const
    {
        return &m_pipelineProcessor->get_command_buffer_for_collecting();
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

    void GPURenderer::upload_camera_transformations( const std::vector<RenderContextFrameData>& render_ctx_data )
    {
        IE_ASSERT( m_sdlGPUDevice != nullptr );
        IE_ASSERT( m_CameraMatrixTransferBuffer != nullptr );
        IE_ASSERT( m_CameraMatrixStorageBuffer != nullptr );

        if ( render_ctx_data.size() == 0 )
            return;

        DXSM::Matrix* buffer_data = static_cast<DXSM::Matrix*>( SDL_MapGPUTransferBuffer( m_sdlGPUDevice, m_CameraMatrixTransferBuffer, true ) );

        for ( const auto render_ctx : render_ctx_data ) {
            *buffer_data = render_ctx.ViewProjectionMatrix;
            ++buffer_data;
        }

        SDL_UnmapGPUTransferBuffer( m_sdlGPUDevice, m_CameraMatrixTransferBuffer );
        SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_CameraMatrixTransferBuffer, .offset = 0 };
        SDL_GPUBufferRegion           bufferRegion { .buffer = m_CameraMatrixStorageBuffer,
                                                     .offset = 0,
                                                     .size   = static_cast<uint32_t>( render_ctx_data.size() * sizeof( DXSM::Matrix ) ) };

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

    void GPURenderer::render_custom_passes( SDL_GPUCommandBuffer* gpu_cmd_buf, const RenderCommandBuffer& render_cmd_buf )
    {
        // custom rendertarget passes
        for ( const auto& render_ctx : render_cmd_buf.RenderContextData ) {
            if ( render_ctx.RenderTarget == nullptr ) {
                continue;
            }

            if ( m_pipelineProcessor->prepare_opaque( render_ctx ) > 0 ) {
                SDL_GPUColorTargetInfo color_target = {};
                color_target.texture                = render_ctx.RenderTarget->get_sdltexture();
                color_target.clear_color            = { render_ctx.ClearColor.R(), render_ctx.ClearColor.G(), render_ctx.ClearColor.B(), render_ctx.ClearColor.A() };
                color_target.load_op                = SDL_GPU_LOADOP_CLEAR;
                color_target.store_op               = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, nullptr );
                SDL_BindGPUVertexStorageBuffers( render_pass, 0, &m_CameraMatrixStorageBuffer, 1 );
                m_pipelineProcessor->render( render_ctx, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );
            }

            if ( m_pipelineProcessor->prepare( render_ctx ) > 0 ) {
                SDL_GPUColorTargetInfo color_target = {};
                color_target.texture                = render_ctx.RenderTarget->get_sdltexture();
                color_target.load_op                = SDL_GPU_LOADOP_LOAD;
                color_target.store_op               = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, nullptr );
                SDL_BindGPUVertexStorageBuffers( render_pass, 0, &m_CameraMatrixStorageBuffer, 1 );
                m_pipelineProcessor->render( render_ctx, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );
            }
        }
    }

    void GPURenderer::render_swapchain_passes( SDL_GPUCommandBuffer* gpu_cmd_buf, const RenderCommandBuffer& render_cmd_buf )
    {
        SDL_GPUTexture* swapchainTexture = nullptr;
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
            color_target.clear_color            = { render_cmd_buf.ClearColor.R(), render_cmd_buf.ClearColor.G(), render_cmd_buf.ClearColor.B(), render_cmd_buf.ClearColor.A() };
            color_target.load_op                = render_cmd_buf.Clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            color_target.store_op               = SDL_GPU_STOREOP_STORE;

            // opaque passes
            for ( const auto& render_ctx_data : render_cmd_buf.RenderContextData ) {
                if ( render_ctx_data.RenderTarget != nullptr )
                    continue;

                uint32_t batch_count = m_pipelineProcessor->prepare_opaque( render_ctx_data );
                if ( batch_count == 0 )
                    continue;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, &depth_stencil );
                SDL_BindGPUVertexStorageBuffers( render_pass, 0, &m_CameraMatrixStorageBuffer, 1 );
                m_pipelineProcessor->render( render_ctx_data, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );

                depth_stencil.texture  = m_DepthTexture;
                depth_stencil.load_op  = SDL_GPU_LOADOP_LOAD;
                depth_stencil.store_op = SDL_GPU_STOREOP_STORE;

                color_target.texture  = swapchainTexture;
                color_target.load_op  = SDL_GPU_LOADOP_LOAD;
                color_target.store_op = SDL_GPU_STOREOP_STORE;
            }

            // alpha passes
            for ( const auto& render_ctx_data : render_cmd_buf.RenderContextData ) {
                if ( render_ctx_data.RenderTarget != nullptr )
                    continue;

                uint32_t batch_count = m_pipelineProcessor->prepare( render_ctx_data );
                if ( batch_count == 0 )
                    continue;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, &depth_stencil );
                SDL_BindGPUVertexStorageBuffers( render_pass, 0, &m_CameraMatrixStorageBuffer, 1 );
                m_pipelineProcessor->render( render_ctx_data, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );
            }

            // render imgui always topmost and without depth testing
            if ( m_pipelineProcessor->prepare_imgui() > 0 ) {
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target, 1, nullptr );
                m_pipelineProcessor->render_imgui( gpu_cmd_buf, render_pass, m_Statistics.get_producer_data() );
                SDL_EndGPURenderPass( render_pass );
            }
        }
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
