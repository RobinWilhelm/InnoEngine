#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/pipelines/Primitive2DPipeline.h"

#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/graphics/Shader.h"

#include "InnoEngine/AssetManager.h"

namespace InnoEngine
{
    Primitive2DPipeline::~Primitive2DPipeline()
    {
        if ( m_QuadPipeline ) {
            SDL_ReleaseGPUGraphicsPipeline( m_Device, m_QuadPipeline );
            m_QuadPipeline = nullptr;
        }
    }

    Result Primitive2DPipeline::initialize( GPURenderer* renderer, AssetManager* asset_manager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() );

        if ( m_Initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_Device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = asset_manager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        Result res = load_quad_pipeline( window, shaderRepo.get() );

        m_Initialized = true;
        return res;
    }

    Result Primitive2DPipeline::load_quad_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo )
    {
        // load shaders
        auto vertexShaderAsset = shader_repo->require_asset( "QuadBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shader_repo->require_asset( "Color.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, sdl_window );
        colorTargets[ 0 ].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.enable_blend          = true;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo     = {};
        pipelineCreateInfo.vertex_shader                         = vertexShader.get()->get_sdlshader();
        pipelineCreateInfo.fragment_shader                       = fragmentShader.get()->get_sdlshader();
        pipelineCreateInfo.primitive_type                        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.target_info.color_target_descriptions = colorTargets;
        pipelineCreateInfo.target_info.num_color_targets         = 1;

        m_QuadPipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_QuadPipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = QuadBatchSize * sizeof( QuadVertexLayout );

        m_QuadTransferBuffer = SDL_CreateGPUTransferBuffer( m_Device, &tbufferCreateInfo );
        if ( m_QuadTransferBuffer == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUTransferBuffer!" );
            return Result::InitializationError;
        }
        m_batches.reserve( 100 );
    }
}    // namespace InnoEngine
