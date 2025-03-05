#include "iepch.h"
#include "Renderer.h"

#include "CoreAPI.h"
#include "Application.h"
#include "Sprite2DPipeline.h"
#include "Sprite.h"
#include "Window.h"

namespace InnoEngine
{
    GPURenderer::~GPURenderer()
    {
        // destory deviceobjects (pipelines etc) before destroyinf gpudevice!
        m_loadedPipelines.clear();

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

    auto GPURenderer::create( Window* pWindow ) -> std::optional<Owned<GPURenderer>>
    {
        Owned<GPURenderer> renderer( new GPURenderer() );

#ifdef _DEBUG
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr );
#else
        renderer->m_sdlGPUDevice = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, nullptr );
#endif

        if ( renderer->m_sdlGPUDevice == nullptr ) {
            IE_LOG_CRITICAL( "GPUCreateDevice failed" );
            return std::nullopt;
        }

        renderer->m_window = pWindow;
        if ( renderer->m_window ) {
            if ( !SDL_ClaimWindowForGPUDevice( renderer->m_sdlGPUDevice, renderer->m_window->get_sdlwindow() ) ) {
                IE_LOG_CRITICAL( "GPUClaimWindow failed" );
                return std::nullopt;
            }
        }

        renderer->retrieve_shaderformatinfo();
        return renderer;
    }

    bool GPURenderer::process_pipelines()
    {
        // simulate some work
        std::this_thread::sleep_for( 2ms );

        for ( auto pipeline : m_renderPipelines ) {
            pipeline->prepare_render( m_sdlGPUDevice );
        }

        if ( m_renderPipelines.size() != 0 ) {
            do_renderpass();
        }

        end_frame();
        return true;
    }

    Window* GPURenderer::get_window() const
    {
        return m_window;
    }

    SDL_GPUDevice* GPURenderer::get_gpudevice() const
    {
        return m_sdlGPUDevice;
    };

    void GPURenderer::set_camera_matrix( const DXSM::Matrix viewProjection )
    {
        m_viewProjection = viewProjection;
    }

    void GPURenderer::end_frame()
    {
        m_renderPipelines.clear();
        m_computePipelines.clear();
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

    void GPURenderer::submit_pipelines()
    {
        for ( auto entry : m_loadedPipelines ) {
            GPUPipeline* pipeline = entry.second.get();
            pipeline->submit();
            uint32_t processing = pipeline->needs_processing();

            if ( processing & PipelineCommand::Render )
                m_renderPipelines.push_back( pipeline );
            if ( processing & PipelineCommand::Compute )
                m_computePipelines.push_back( pipeline );
        }
    }

    void GPURenderer::do_renderpass()
    {
        SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer( m_sdlGPUDevice );
        if ( cmdbuf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
        }

        // TODO: custom rendertarget pass
        { }

        // Main swapchain pass
        // only works when a window is associated
        if ( has_window() ) {
            SDL_GPUTexture* swapchainTexture;
            if ( !SDL_WaitAndAcquireGPUSwapchainTexture( cmdbuf, m_window->get_sdlwindow(), &swapchainTexture, nullptr, nullptr ) ) {
                SDL_CancelGPUCommandBuffer( cmdbuf );
                IE_LOG_WARNING( "WaitAndAcquireGPUSwapchainTexture failed : %s", SDL_GetError() );
                return;
            }

            if ( swapchainTexture != nullptr ) {
                SDL_GPUColorTargetInfo colorTargetInfo = {};
                colorTargetInfo.texture                = swapchainTexture;
                colorTargetInfo.clear_color            = { 0.0f, 0.5f, 0.0f, 1.0f };
                colorTargetInfo.load_op                = SDL_GPU_LOADOP_CLEAR;
                colorTargetInfo.store_op               = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass( cmdbuf, &colorTargetInfo, 1, nullptr );

                for ( auto pipeline : m_renderPipelines ) {
                    pipeline->swapchain_render( m_viewProjection, cmdbuf, renderPass );
                }

                SDL_EndGPURenderPass( renderPass );
            }
        }

        if ( SDL_SubmitGPUCommandBuffer( cmdbuf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed : %s", SDL_GetError() );
            return;
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
