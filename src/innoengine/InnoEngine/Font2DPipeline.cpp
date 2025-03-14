#include "InnoEngine/iepch.h"
#include "InnoEngine/Font2DPipeline.h"

#include "InnoEngine/Renderer.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/Shader.h"

#include "InnoEngine/Window.h"

#include "RenderCommandBuffer.h"

namespace InnoEngine
{
    Font2DPipeline::~Font2DPipeline()
    {
        if ( m_device != nullptr ) {
            if ( m_transferBuffer ) {
                SDL_ReleaseGPUTransferBuffer( m_device, m_transferBuffer );
                m_transferBuffer = nullptr;
            }

            for ( auto gpubuffer : m_gpuBuffer ) {
                SDL_ReleaseGPUBuffer( m_device, gpubuffer );
            }
            m_gpuBuffer.clear();
            m_batches.clear();

            if ( m_fontSampler ) {
                SDL_ReleaseGPUSampler( m_device, m_fontSampler );
                m_fontSampler = nullptr;
            }

            if ( m_pipeline ) {
                SDL_ReleaseGPUGraphicsPipeline( m_device, m_pipeline );
                m_pipeline = nullptr;
            }
        }
    }

    Result Font2DPipeline::initialize( GPURenderer* renderer, AssetManager* assetmanager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() && assetmanager != nullptr );

        if ( m_initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = assetmanager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        // load shaders
        auto vertexShaderAsset = shaderRepo->require_asset( "SpriteBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shaderRepo->require_asset( "MSDFFont.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_device, window );
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

        m_pipeline = SDL_CreateGPUGraphicsPipeline( m_device, &pipelineCreateInfo );
        if ( m_pipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = MaxBatchSize * sizeof( Command::VertexUniform );

        m_transferBuffer = SDL_CreateGPUTransferBuffer( m_device, &tbufferCreateInfo );
        if ( m_transferBuffer == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUTransferBuffer!" );
            return Result::InitializationError;
        }

        SDL_GPUSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.min_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mag_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler_create_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_fontSampler = SDL_CreateGPUSampler( m_device, &sampler_create_info );
        if ( m_fontSampler == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUSampler!" );
            return Result::InitializationError;
        }

        m_batches.reserve( 100 );
        m_initialized = true;
        return Result::Success;
    }

    void Font2DPipeline::prepare_render( const CommandList& command_list, const FontList& font_list )
    {
        IE_ASSERT( m_device != nullptr );
        sort_commands( command_list );

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }


        (void) font_list;

    }

    void Font2DPipeline::swapchain_render( const DXSM::Matrix& view_projection, const CommandList& command_list, const FontList& font_list, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass )
    {
        (void)view_projection;
        (void)command_list;
        (void)font_list;
        (void)cmdbuf;
        (void)renderPass;
    }

    Font2DPipeline::BatchData* Font2DPipeline::add_batch()
    {
        BatchData& newbatch   = m_batches.emplace_back();
        newbatch.buffer_index = static_cast<uint16_t>( find_free_gpubuffer() );
        newbatch.count        = 0;
        return &newbatch;
    }

    void Font2DPipeline::clear_batches()
    {
        m_batches.clear();
        m_gpuBuffer_used = 0;
    }

    uint32_t Font2DPipeline::find_free_gpubuffer()
    {
        if ( m_gpuBuffer_used < m_gpuBuffer.size() )
            return m_gpuBuffer_used++;

        auto&                   buffer     = m_gpuBuffer.emplace_back();
        SDL_GPUBufferCreateInfo createInfo = {};
        createInfo.usage                   = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        createInfo.size                    = MaxBatchSize * sizeof( Command::VertexUniform );

        buffer = SDL_CreateGPUBuffer( m_device, &createInfo );
        if ( buffer == nullptr ) {
            // TODO: this needs to be handled better
            IE_LOG_ERROR( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() );
            return 0;
        }
        return m_gpuBuffer_used++;
    }

    SDL_GPUBuffer* Font2DPipeline::get_gpubuffer_by_index( uint32_t index ) const
    {
        IE_ASSERT( index < m_gpuBuffer_used );
        return m_gpuBuffer[ index ];
    }

    void Font2DPipeline::sort_commands( const CommandList& command_list )
    {
        m_sortedCommands.clear();

        if ( command_list.size() > m_sortedCommands.size() )
            m_sortedCommands.resize( command_list.size() );

        for ( size_t i = 0; i < command_list.size(); ++i ) {
            m_sortedCommands[ i ] = &command_list[ i ];
        }

        std::sort( m_sortedCommands.begin(), m_sortedCommands.end(), []( const Command* a, const Command* b ) {
            if ( a->font_fbidx < b->font_fbidx )
                return true;

            if ( a->info.z > b->info.z )
                return true;

            return false;
        } );
    }
}    // namespace InnoEngine
