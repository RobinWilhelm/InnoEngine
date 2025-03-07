#include "iepch.h"
#include "Renderer.h"

#include "CoreAPI.h"
#include "Application.h"
#include "Window.h"

#include "Texture2D.h"

#include "Sprite2DPipeline.h"
#include "ImGuiPipeline.h"

namespace InnoEngine
{
    struct RenderCommandBuffer
    {
        RenderCommandBuffer()
        {
            SpriteCommandBuffer.reserve( 20000 );
        }

        DXSM::Matrix                           CameraMatrix;
        std::vector<Sprite2DPipeline::Command> SpriteCommandBuffer;
        ImGuiPipeline::CommandData             ImGuiCommandBuffer;
    };

    class GPURenderer::PipelineProcessor
    {
    public:
        void initialize( GPURenderer* renderer, AssetManager* assetmanager )
        {
            IE_ASSERT( renderer != nullptr && assetmanager != nullptr );

            m_sprite2DPipeline = std::make_unique<Sprite2DPipeline>();
            IE_ASSERT( IE_SUCCESS( m_sprite2DPipeline->initialize( renderer, assetmanager ) ) );

            m_imguiPipeline = std::make_unique<ImGuiPipeline>();
            IE_ASSERT( IE_SUCCESS( m_imguiPipeline->initialize( renderer ) ) );
            m_initialized = true;
        }

        void prepare( SDL_GPUDevice* device )
        {
            IE_ASSERT( m_initialized );
            RenderCommandBuffer& render_cmd_buf = m_renderCommandBuffer.get_second();
            m_sprite2DPipeline->prepare_render( render_cmd_buf.SpriteCommandBuffer, device );
            m_imguiPipeline->prepare_render( render_cmd_buf.ImGuiCommandBuffer, device );
        }

        void render( SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass )
        {
            IE_ASSERT( m_initialized );
            RenderCommandBuffer& render_cmd_buf = m_renderCommandBuffer.get_second();
            m_sprite2DPipeline->swapchain_render( render_cmd_buf.CameraMatrix, render_cmd_buf.SpriteCommandBuffer, gpu_cmd_buf, render_pass );
            m_imguiPipeline->swapchain_render( render_cmd_buf.ImGuiCommandBuffer, gpu_cmd_buf, render_pass );
        }

        RenderCommandBuffer& get_command_buffer()
        {
            return m_renderCommandBuffer.get_first();
        }

        void on_synchronize()
        {
            m_renderCommandBuffer.swap();
        }

    private:
        bool                                m_initialized = false;
        DoubleBuffered<RenderCommandBuffer> m_renderCommandBuffer;
        Own<Sprite2DPipeline>               m_sprite2DPipeline;
        Own<ImGuiPipeline>                  m_imguiPipeline;
    };

    GPURenderer::~GPURenderer()
    {
        if ( m_sdlGPUDevice ) {
            SDL_WaitForGPUIdle( m_sdlGPUDevice );
        }

        if ( m_window ) {
            SDL_ReleaseWindowFromGPUDevice( m_sdlGPUDevice, m_window->get_sdlwindow() );
            m_window = nullptr;
        }

        if ( m_sdlGPUDevice ) {
            SDL_DestroyGPUDevice( m_sdlGPUDevice );
            m_sdlGPUDevice = nullptr;
        }
    }

    auto GPURenderer::create( Window* window, AssetManager* assetmanager ) -> std::optional<Own<GPURenderer>>
    {
        Own<GPURenderer> renderer( new GPURenderer() );

#ifdef _DEBUG
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr );
#else
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, nullptr );
#endif

        if ( renderer->m_sdlGPUDevice == nullptr ) {
            IE_LOG_CRITICAL( "GPUCreateDevice failed" );
            return std::nullopt;
        }

        renderer->m_window = window;
        if ( renderer->m_window ) {
            if ( !SDL_ClaimWindowForGPUDevice( renderer->m_sdlGPUDevice, renderer->m_window->get_sdlwindow() ) ) {
                IE_LOG_CRITICAL( "GPUClaimWindow failed" );
                return std::nullopt;
            }
        }

        renderer->m_pipelineProcessor = std::make_unique<PipelineProcessor>();
        renderer->m_pipelineProcessor->initialize( renderer.get(), assetmanager );
        renderer->retrieve_shaderformatinfo();
        return renderer;
    }

    Window* GPURenderer::get_window() const
    {
        return m_window;
    }

    SDL_GPUDevice* GPURenderer::get_gpudevice() const
    {
        return m_sdlGPUDevice;
    };

    void GPURenderer::set_camera_matrix( const DXSM::Matrix view_projection )
    {
        m_pipelineProcessor->get_command_buffer().CameraMatrix = view_projection;
    }

    ShaderFormatInfo GPURenderer::get_needed_shaderformat()
    {
        return m_shaderFormat;
    }

    std::string GPURenderer::add_shaderformat_fileextension( std::string_view name )
    {
        return std::string( name ).append( m_shaderFormat.FileNameExtension );
    }

    bool GPURenderer::has_window()
    {
        return m_window != nullptr;
    }

    bool GPURenderer::enable_vsync( bool enabled )
    {
        if ( enabled == false ) {
            bool supported = SDL_WindowSupportsGPUPresentMode( m_sdlGPUDevice, get_window()->get_sdlwindow(), SDL_GPU_PRESENTMODE_IMMEDIATE );
            if ( supported == false )
                return false;
        }

        return SDL_SetGPUSwapchainParameters( m_sdlGPUDevice, get_window()->get_sdlwindow(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, enabled ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE );
    }

    void GPURenderer::on_synchronize()
    {
        m_pipelineProcessor->on_synchronize();
    }

    void GPURenderer::render()
    {
        m_pipelineProcessor->prepare( get_gpudevice() );

        SDL_GPUCommandBuffer* gpu_cmd_buf = SDL_AcquireGPUCommandBuffer( m_sdlGPUDevice );
        if ( gpu_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }

        // TODO: custom rendertarget pass
        {
        }

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
                color_target_info.clear_color            = { 0.0f, 0.5f, 0.0f, 1.0f };
                color_target_info.load_op                = SDL_GPU_LOADOP_CLEAR;
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

    void GPURenderer::add_sprite( const Sprite& sprite )
    {
        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer();

        Sprite2DPipeline::Command& cmd = render_cmd_buf.SpriteCommandBuffer.emplace_back();
        cmd.texture                    = sprite.m_texture->get_uid();
        cmd.info.x                     = sprite.m_position.x;
        cmd.info.y                     = sprite.m_position.y;
        cmd.info.z                     = sprite.m_layer;
        cmd.info.rotation              = sprite.m_rotation;
        cmd.info.scale_w               = sprite.m_scale.x;
        cmd.info.scale_h               = sprite.m_scale.y;
        cmd.info.source                = sprite.m_sourceRect;
        cmd.info.color                 = sprite.m_color;
    }

    void GPURenderer::add_imgui_draw_data( ImDrawData* draw_data )
    {
        IE_ASSERT( draw_data != nullptr );
        auto& render_cmd_buf = m_pipelineProcessor->get_command_buffer();

        ImGuiPipeline::CommandData& cmd = render_cmd_buf.ImGuiCommandBuffer;
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

    void GPURenderer::retrieve_shaderformatinfo()
    {
        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats( m_sdlGPUDevice );
        if ( backendFormats & SDL_GPU_SHADERFORMAT_SPIRV ) {
            m_shaderFormat.SubDirectory      = "SPIRV";
            m_shaderFormat.Format            = SDL_GPU_SHADERFORMAT_SPIRV;
            m_shaderFormat.EntryPoint        = "main";
            m_shaderFormat.FileNameExtension = ".spv";
        }
        else if ( backendFormats & SDL_GPU_SHADERFORMAT_MSL ) {
            m_shaderFormat.SubDirectory      = "MSL";
            m_shaderFormat.Format            = SDL_GPU_SHADERFORMAT_MSL;
            m_shaderFormat.EntryPoint        = "main0";
            m_shaderFormat.FileNameExtension = ".msl";
        }
        else if ( backendFormats & SDL_GPU_SHADERFORMAT_DXIL ) {
            m_shaderFormat.SubDirectory      = "DXIL";
            m_shaderFormat.Format            = SDL_GPU_SHADERFORMAT_DXIL;
            m_shaderFormat.EntryPoint        = "main";
            m_shaderFormat.FileNameExtension = ".dxil";
        }
    }
}    // namespace InnoEngine
