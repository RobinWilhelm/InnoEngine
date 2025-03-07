#include "iepch.h"
#include "Sprite2DPipeline.h"

#include "Application.h"
#include "Asset.h"
#include "Sprite.h"
#include "AssetManager.h"
#include "CoreAPI.h"
#include "RenderCommandBuffer.h"
#include "Renderer.h"
#include "Window.h"

#include "Shader.h"
#include "AssetRepository.h"

namespace InnoEngine
{
    static constexpr uint32_t SpriteBatchSizeMax = 20000;

    Sprite2DPipeline::~Sprite2DPipeline()
    {
        if ( m_spriteTransferBuffer ) {
            SDL_ReleaseGPUTransferBuffer( m_renderer->get_gpudevice(), m_spriteTransferBuffer );
            m_spriteTransferBuffer = nullptr;
        }

        for ( auto gpubuffer : m_gpuBuffer ) {
            SDL_ReleaseGPUBuffer( m_renderer->get_gpudevice(), gpubuffer );
        }
        m_gpuBuffer.clear();
        m_batches.clear();
    }

    Result Sprite2DPipeline::initialize( GPURenderer* renderer, AssetManager* assetmanager )
    {
        IE_ASSERT( renderer != nullptr && assetmanager != nullptr);

        if ( m_initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_renderer = renderer;

        auto shaderRepo = assetmanager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        // load shaders
        auto vertexShaderAsset = shaderRepo->require_asset( m_renderer->add_shaderformat_fileextension( "SpriteBatch.vert" ) );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shaderRepo->require_asset( m_renderer->add_shaderformat_fileextension( "TextureXColor.frag" ) );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        IE_ASSERT( IE_SUCCESS( vertexShader.get()->create_device_ressources( m_renderer, { 0, 0, 1, 1 } ) ) );
        IE_ASSERT( IE_SUCCESS( fragmentShader.get()->create_device_ressources( m_renderer, { 1, 0, 0, 0 } ) ) );

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_renderer->get_gpudevice(), m_renderer->get_window()->get_sdlwindow() );
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

        m_pipeline = SDL_CreateGPUGraphicsPipeline( m_renderer->get_gpudevice(), &pipelineCreateInfo );
        if ( m_pipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = SpriteBatchSizeMax * sizeof( Command::VertexUniform );

        m_spriteTransferBuffer = SDL_CreateGPUTransferBuffer( m_renderer->get_gpudevice(), &tbufferCreateInfo );
        if ( m_spriteTransferBuffer == nullptr ) {
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

        m_defaultSampler = SDL_CreateGPUSampler( m_renderer->get_gpudevice(), &sampler_create_info );
        if ( m_defaultSampler == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUSampler!" );
            return Result::InitializationError;
        }

        m_textureAssets = assetmanager->get_repository<Texture2D>();
        m_batches.reserve( 100 );

        m_initialized = true;
        return Result::Success;
    }

    void Sprite2DPipeline::prepare_render( const CommandList& command_list, SDL_GPUDevice* gpudevice )
    {
        IE_ASSERT( gpudevice );
        sort_commands( command_list );

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( gpudevice );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        Command::VertexUniform* uniform_data = nullptr;
        AssetUID<Texture2D>     current_texture;

        bool       first_batch   = true;
        BatchData* current_batch = nullptr;

        for ( const Command* command : m_sortedCommands ) {
            // start new batch when changing texture or max batch size is reached (should be sorted by texture at this point)
            if ( current_batch == nullptr || current_texture != command->texture || current_batch->count >= SpriteBatchSizeMax ) {
                // unmap and upload previous batch data before changeing to new batch
                if ( first_batch == false ) {
                    SDL_UnmapGPUTransferBuffer( gpudevice, m_spriteTransferBuffer );
                    SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_spriteTransferBuffer, .offset = 0 };
                    SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( current_batch->bufferIdx ),
                                                                 .offset = 0,
                                                                 .size   = static_cast<uint32_t>( current_batch->count * sizeof( Command::VertexUniform ) ) };
                    SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );
                    uniform_data = nullptr;
                }

                current_batch          = add_batch();
                current_batch->texture = command->texture;
                current_texture        = command->texture;

                uniform_data = static_cast<Command::VertexUniform*>( SDL_MapGPUTransferBuffer( gpudevice, m_spriteTransferBuffer, true ) );
                first_batch  = false;
            }

            // add to batch
            uniform_data[ current_batch->count++ ] = command->info;
        }

        // unmap and upload last batch data
        if ( current_batch && current_batch->count > 0 ) {
            SDL_UnmapGPUTransferBuffer( m_renderer->get_gpudevice(), m_spriteTransferBuffer );
            SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_spriteTransferBuffer, .offset = 0 };
            SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( current_batch->bufferIdx ),
                                                         .offset = 0,
                                                         .size   = static_cast<uint32_t>( current_batch->count * sizeof( Command::VertexUniform ) ) };
            SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );
        }

        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    void Sprite2DPipeline::swapchain_render( const DXSM::Matrix& view_projection, const CommandList& command_list, SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass )
    {
        (void)command_list;

        IE_ASSERT( m_renderer != nullptr && m_renderer->get_gpudevice() != nullptr );
        IE_ASSERT( gpu_cmd_buf != nullptr && render_pass != nullptr );

        SDL_BindGPUGraphicsPipeline( render_pass, m_pipeline );
        SDL_PushGPUVertexUniformData( gpu_cmd_buf, 0, &view_projection, sizeof( DXSM::Matrix ) );

        for ( size_t i = 0; i < m_batches.size(); ++i ) {
            if ( m_batches[ i ].texture.valid() == false ) {
                continue;
            }

            auto gpuBuffer = get_gpubuffer_by_index( m_batches[ i ].bufferIdx );
            SDL_BindGPUVertexStorageBuffers( render_pass, 0, &gpuBuffer, 1 );

            auto texture = m_textureAssets->get_asset( m_batches[ i ].texture );

            SDL_GPUTextureSamplerBinding texture_sampler_binding = {};
            texture_sampler_binding.sampler = m_defaultSampler;
            texture_sampler_binding.texture = texture->get_sdltexture();

            SDL_BindGPUFragmentSamplers( render_pass, 0, &texture_sampler_binding, 1 );

            SDL_DrawGPUPrimitives( render_pass, m_batches[ i ].count * 6, 1, 0, 0 );
        }
        clear_batches();
    }

    void Sprite2DPipeline::sort_commands( const CommandList& command_list )
    {
        m_sortedCommands.clear();

        if ( command_list.size() > m_sortedCommands.size() )
            m_sortedCommands.resize( command_list.size() );

        for ( size_t i = 0; i < command_list.size(); ++i ) {
            m_sortedCommands[ i ] = &command_list[ i ];
        }

        std::sort( m_sortedCommands.begin(), m_sortedCommands.end(), []( const Command* a, const Command* b ) {
            if ( a->texture < b->texture )
                return true;

            if ( a->info.z > b->info.z )
                return true;

            return false;
        } );
    }

    Sprite2DPipeline::BatchData* Sprite2DPipeline::add_batch()
    {
        Sprite2DPipeline::BatchData& newbatch = m_batches.emplace_back();
        newbatch.bufferIdx                    = find_free_gpubuffer();
        newbatch.count                        = 0;
        return &newbatch;
    }

    void Sprite2DPipeline::clear_batches()
    {
        m_batches.clear();
        m_gpuBuffer_used = 0;
    }

    uint32_t Sprite2DPipeline::find_free_gpubuffer()
    {
        if ( m_gpuBuffer_used < m_gpuBuffer.size() )
            return m_gpuBuffer_used++;

        auto&                   buffer     = m_gpuBuffer.emplace_back();
        SDL_GPUBufferCreateInfo createInfo = {};
        createInfo.usage                   = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        createInfo.size                    = SpriteBatchSizeMax * sizeof( Command::VertexUniform );

        buffer = SDL_CreateGPUBuffer( m_renderer->get_gpudevice(), &createInfo );
        if ( buffer == nullptr ) {
            // TODO: this needs to be handled better
            IE_LOG_ERROR( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() );
            return 0;
        }
        return m_gpuBuffer_used++;
    }

    SDL_GPUBuffer* Sprite2DPipeline::get_gpubuffer_by_index( uint32_t index ) const
    {
        IE_ASSERT( index < m_gpuBuffer_used );
        return m_gpuBuffer[ index ];
    }
}    // namespace InnoEngine
