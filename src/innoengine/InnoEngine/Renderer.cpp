#include "InnoEngine/iepch.h"
#include "InnoEngine/Renderer.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/Window.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/AssetRepository.h"

#include "InnoEngine/Texture2D.h"
#include "InnoEngine/Sprite.h"
#include "InnoEngine/Font.h"

#include "InnoEngine/Shader.h"
#include "InnoEngine/StringArena.h"

#include "RenderCommandBuffer.h"

#include "SDL3/SDL_vulkan.h"

namespace InnoEngine
{
    class GPURenderer::PipelineProcessor
    {
    public:
        void initialize( GPURenderer* renderer, AssetManager* assetmanager )
        {
            IE_ASSERT( renderer != nullptr && assetmanager != nullptr );

            m_sprite2DPipeline = std::make_unique<Sprite2DPipeline>();
            RETURN_IF_FAILED( m_sprite2DPipeline->initialize( renderer, assetmanager ) );

            // m_font2DPipeline = std::make_unique<Font2DPipeline>();
            // RETURN_IF_FAILED( m_font2DPipeline->initialize( renderer, assetmanager ) );

            m_imguiPipeline = std::make_unique<ImGuiPipeline>();
            RETURN_IF_FAILED( m_imguiPipeline->initialize( renderer ) );
            m_initialized = true;
        }

        void prepare( SDL_GPUDevice* device )
        {
            (void)device;
            IE_ASSERT( m_initialized );
            RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();
            m_sprite2DPipeline->prepare_render( render_cmd_buf.SpriteRenderCommands );
            // m_font2DPipeline->prepare_render( render_cmd_buf.FontRenderCommands, render_cmd_buf.FontRegister );
            m_imguiPipeline->prepare_render( render_cmd_buf.ImGuiCommandBuffer );
        }

        void render( SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass )
        {
            IE_ASSERT( m_initialized );
            (void)gpu_cmd_buf;
            (void)render_pass;

            RenderCommandBuffer& render_cmd_buf = get_command_buffer_for_rendering();
            m_sprite2DPipeline->swapchain_render( render_cmd_buf.CameraMatrix, render_cmd_buf.SpriteRenderCommands, render_cmd_buf.TextureRegister, gpu_cmd_buf, render_pass );
            // m_font2DPipeline->swapchain_render( render_cmd_buf.CameraMatrix, render_cmd_buf.FontRenderCommands, render_cmd_buf.FontRegister, gpu_cmd_buf, render_pass );
            m_imguiPipeline->swapchain_render( render_cmd_buf.ImGuiCommandBuffer, gpu_cmd_buf, render_pass );
        }

        RenderCommandBuffer& get_command_buffer_for_collecting()
        {
            return m_renderCommandBuffer.get_first();
        }

        RenderCommandBuffer& get_command_buffer_for_rendering()
        {
            return m_renderCommandBuffer.get_second();
        }

        void on_synchronize()
        {
            m_renderCommandBuffer.swap();
            get_command_buffer_for_collecting().clear();
        }

    private:
        DoubleBuffered<RenderCommandBuffer> m_renderCommandBuffer;
        Own<Sprite2DPipeline>               m_sprite2DPipeline;
        Own<Font2DPipeline>                 m_font2DPipeline;
        Own<ImGuiPipeline>                  m_imguiPipeline;
        bool                                m_initialized = false;
    };

    GPURenderer::~GPURenderer()
    {
        if ( m_sdlGPUDevice ) {
            wait_for_gpu_idle();

            m_pipelineProcessor.reset();

            if ( m_window ) {
                SDL_ReleaseWindowFromGPUDevice( m_sdlGPUDevice, m_window->get_sdlwindow() );
                m_window = nullptr;
            }

#ifdef _DEBUG
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

        bool res = SDL_Vulkan_LoadLibrary( nullptr );
        (void)res;

        const char* driver = nullptr;
#ifdef _DEBUG
        renderer->m_sdlGPUDevice = GPUDeviceRef::create( SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, driver ) );
#else
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, driver );
#endif

        if ( renderer->m_sdlGPUDevice == nullptr ) {
            IE_LOG_CRITICAL( " SDL_CreateGPUDevice failed! {}", SDL_GetError() );
            return std::nullopt;
        }

        IE_LOG_DEBUG( "Selected gpu driver: {}", renderer->get_devicedriver() );

        if ( renderer->m_sdlGPUDevice == nullptr ) {
            IE_LOG_CRITICAL( "GPUCreateDevice failed" );
            return std::nullopt;
        }

        renderer->m_pipelineProcessor = std::make_unique<PipelineProcessor>();
        renderer->retrieve_shaderformatinfo();
        return renderer;
    }

    Result GPURenderer::initialize( Window* window, AssetManager* assetmanager )
    {
        if ( m_initialized ) {
            IE_LOG_WARNING( "GPURenderer: Trying to initialize more than once!" );
            return Result::AlreadyInitialized;
        }

        m_window = window;
        if ( m_window ) {
            if ( !SDL_ClaimWindowForGPUDevice( m_sdlGPUDevice, m_window->get_sdlwindow() ) ) {
                IE_LOG_CRITICAL( "GPUClaimWindow failed" );
                return Result::InitializationError;
            }
        }

        m_pipelineProcessor->initialize( this, assetmanager );
        m_initialized = true;
        return Result::Success;
    }

    Window* GPURenderer::get_window() const
    {
        return m_window;
    }

    GPUDeviceRef GPURenderer::get_gpudevice() const
    {
        return m_sdlGPUDevice;
    };

    bool GPURenderer::has_window()
    {
        return m_window != nullptr;
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

    void GPURenderer::wait_for_gpu_idle()
    {
        SDL_WaitForGPUIdle( m_sdlGPUDevice );
    }

    void GPURenderer::on_synchronize()
    {
#ifdef _DEBUG
        // reset the frame indices so they can be checked to catch errors
        auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();
        for ( auto& texture : render_commands.TextureRegister )
            texture->m_frameBufferIndex = -1;

        for ( auto& font : render_commands.FontRegister )
            font->m_frameBufferIndex = -1;
#endif
        m_pipelineProcessor->on_synchronize();
    }

    void GPURenderer::render()
    {
        IE_ASSERT( m_initialized );

        // dont render when minimized
        if ( m_window && SDL_GetWindowFlags( m_window->get_sdlwindow() ) & SDL_WINDOW_MINIMIZED ) {
            return;
        }

        const auto& render_commands = m_pipelineProcessor->get_command_buffer_for_rendering();
        m_pipelineProcessor->prepare( get_gpudevice() );

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
            if ( !SDL_WaitAndAcquireGPUSwapchainTexture( gpu_cmd_buf, m_window->get_sdlwindow(), &swapchainTexture, nullptr, nullptr ) ) {
                SDL_CancelGPUCommandBuffer( gpu_cmd_buf );
                IE_LOG_WARNING( "WaitAndAcquireGPUSwapchainTexture failed : %s", SDL_GetError() );
                return;
            }

            if ( swapchainTexture != nullptr ) {
                SDL_GPUColorTargetInfo color_target_info = {};
                color_target_info.texture                = swapchainTexture;
                color_target_info.clear_color            = { render_commands.ClearColor.R(), render_commands.ClearColor.G(), render_commands.ClearColor.B(), render_commands.ClearColor.A() };
                color_target_info.load_op                = render_commands.Clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
                color_target_info.store_op               = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass( gpu_cmd_buf, &color_target_info, 1, nullptr );
                m_pipelineProcessor->render( gpu_cmd_buf, render_pass );
                SDL_EndGPURenderPass( render_pass );
            }
        }

        if ( SDL_SubmitGPUCommandBuffer( gpu_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }
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
        IE_ASSERT( font != nullptr && font->m_initialized );

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

    void GPURenderer::set_view_projection( const DXSM::Matrix view_projection )
    {
        auto& render_cmd_buf        = m_pipelineProcessor->get_command_buffer_for_collecting();
        render_cmd_buf.CameraMatrix = view_projection;
    }

    void GPURenderer::add_sprite( const Sprite& sprite )
    {
        IE_ASSERT( sprite.m_texture != nullptr && sprite.m_texture->m_frameBufferIndex >= 0 );

        RenderCommandBuffer&       render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Sprite2DPipeline::Command& cmd            = render_cmd_buf.SpriteRenderCommands.emplace_back();

        cmd.texture_index        = sprite.m_texture->m_frameBufferIndex;
        cmd.info.x               = sprite.m_position.x;
        cmd.info.y               = sprite.m_position.y;
        cmd.info.z               = sprite.m_layer;
        cmd.info.rotation        = DirectX::XMConvertToRadians( sprite.m_rotationDegrees );
        cmd.info.width           = sprite.m_scale.x * sprite.m_texture->m_width;
        cmd.info.height          = sprite.m_scale.y * sprite.m_texture->m_height;
        cmd.info.origin_offset_x = sprite.m_originOffset.x * cmd.info.width;
        cmd.info.origin_offset_y = sprite.m_originOffset.y * cmd.info.height;
        cmd.info.source          = sprite.m_sourceRect;
        cmd.info.color           = sprite.m_color;
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

    const RenderCommandBuffer* GPURenderer::get_render_command_buffer() const
    {
        return &m_pipelineProcessor->get_command_buffer_for_collecting();
    }

    void GPURenderer::add_text( const Font* font, float x, float y, std::string_view text, DXSM::Color color, uint16_t layer, float scale )
    {
        IE_ASSERT( font != nullptr && font->m_frameBufferIndex >= 0 );

        RenderCommandBuffer&     render_cmd_buf = m_pipelineProcessor->get_command_buffer_for_collecting();
        Font2DPipeline::Command& cmd            = render_cmd_buf.FontRenderCommands.emplace_back();

        cmd.font_fbidx         = font->m_frameBufferIndex;
        cmd.string_arena_index = render_cmd_buf.StringBuffer.insert( text );
        cmd.info.x             = x;
        cmd.info.y             = y;
        cmd.info.z             = layer;
        cmd.info.color         = color;
        cmd.info.width         = scale;
        cmd.info.height        = scale;
        cmd.info.rotation      = 0.0f;
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
