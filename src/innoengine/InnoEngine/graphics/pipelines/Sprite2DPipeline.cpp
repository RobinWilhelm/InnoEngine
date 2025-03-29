#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/pipelines/Sprite2DPipeline.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/Application.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/graphics/RenderContext.h"

#include "InnoEngine/Asset.h"
#include "InnoEngine/graphics/Sprite.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/Shader.h"

namespace InnoEngine
{
    Sprite2DPipeline::~Sprite2DPipeline()
    {
        if ( m_Device != nullptr ) {
            if ( m_DefaultSampler ) {
                SDL_ReleaseGPUSampler( m_Device, m_DefaultSampler );
                m_DefaultSampler = nullptr;
            }

            if ( m_Pipeline ) {
                SDL_ReleaseGPUGraphicsPipeline( m_Device, m_Pipeline );
                m_Pipeline = nullptr;
            }
        }
    }

    Result Sprite2DPipeline::initialize( GPURenderer* renderer, AssetManager* assetmanager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() && assetmanager != nullptr );

        if ( m_Initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_Device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = assetmanager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        // load shaders
        auto vertexShaderAsset = shaderRepo->require_asset( "SpriteBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shaderRepo->require_asset( "TextureXColor.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, window );
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

        pipelineCreateInfo.target_info.depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.compare_op          = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test   = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write  = true;
        pipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
        pipelineCreateInfo.depth_stencil_state.write_mask          = 0xFF;

        m_Pipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_Pipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.min_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mag_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler_create_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_DefaultSampler = SDL_CreateGPUSampler( m_Device, &sampler_create_info );
        if ( m_DefaultSampler == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUSampler!" );
            return Result::InitializationError;
        }

        m_GPUBatch = GPUBatchStorageBuffer<StructuredBufferLayout, BatchData>::create( m_Device, MaxBatchSize );

        m_Initialized = true;
        return Result::Success;
    }

    void Sprite2DPipeline::prepare_render( const CommandList& command_list )
    {
        IE_ASSERT( m_Device != nullptr );

        sort_commands( command_list );
        m_GPUBatch->clear();

        if ( command_list.size() == 0 )
            return;

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_Device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        BatchData* current = nullptr;

        for ( const Command* command : m_SortedCommands ) {
            if ( current == nullptr || m_GPUBatch->current_batch_full() ||
                 current->ContextIndex != command->ContextIndex ||
                 current->TextureIndex != command->TextureIndex ) {

                current               = m_GPUBatch->upload_and_add_batch( copy_pass );
                current->ContextIndex = command->ContextIndex;
                current->TextureIndex = command->TextureIndex;
            }

            StructuredBufferLayout* buffer_data = m_GPUBatch->next_data();

            buffer_data->ContextIndex = command->ContextIndex;
            buffer_data->Color        = command->Color;
            buffer_data->Position     = command->Position;
            buffer_data->OriginOffset = command->OriginOffset;
            buffer_data->Size         = command->Size;
            buffer_data->Rotation     = command->Rotation;
            buffer_data->Depth        = command->Depth;
            buffer_data->SourceRect   = command->SourceRect;
        }
        m_GPUBatch->upload_last( copy_pass );

        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    uint32_t Sprite2DPipeline::swapchain_render( const RenderContextFrameData& render_ctx_data,
                                                 const TextureList&            texture_list,
                                                 SDL_GPURenderPass*            render_pass )
    {
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( render_pass != nullptr );

        if ( m_GPUBatch->size() == 0 )
            return 0;

        SDL_BindGPUGraphicsPipeline( render_pass, m_Pipeline );
        SDL_BindGPUVertexBuffers( render_pass, 0, nullptr, 0 );
        SDL_SetGPUViewport( render_pass, &render_ctx_data.Viewport );

        uint32_t                     draw_calls      = 0;
        RenderCommandBufferIndexType current_texture = InvalidRenderCommandBufferIndex;

        for ( const auto& batch_data : m_GPUBatch->get_batchlist() ) {
            if ( batch_data.CustomData.TextureIndex != current_texture ) {
                SDL_GPUTextureSamplerBinding texture_sampler_binding = {};
                texture_sampler_binding.sampler                      = m_DefaultSampler;
                texture_sampler_binding.texture                      = texture_list[ batch_data.CustomData.TextureIndex ]->get_sdltexture();
                SDL_BindGPUFragmentSamplers( render_pass, 0, &texture_sampler_binding, 1 );
                current_texture = batch_data.CustomData.TextureIndex;
            }

            SDL_BindGPUVertexStorageBuffers( render_pass, 1, &batch_data.GPUBuffer, 1 );
            SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
            ++draw_calls;
        }

        return draw_calls;
    }

    void Sprite2DPipeline::sort_commands( const CommandList& command_list )
    {
        m_SortedCommands.clear();

        if ( command_list.size() > m_SortedCommands.size() )
            m_SortedCommands.reserve( command_list.size() );

        for ( size_t i = 0; i < command_list.size(); ++i ) {
            m_SortedCommands.push_back( &command_list[ i ] );
        }

        std::sort( m_SortedCommands.begin(), m_SortedCommands.end(), []( const Command* a, const Command* b ) {
            if ( a->TextureIndex > b->TextureIndex )
                return true;

            if ( a->Depth > b->Depth )
                return true;

            return false;
        } );
    }
}    // namespace InnoEngine
