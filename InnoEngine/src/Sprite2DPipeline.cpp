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

bool Sprite2DPipeline::init( GPURenderer* pRenderer )
{
    IE_ASSERT( pRenderer != nullptr );

    if ( m_initialized ) {
        IE_LOG_WARNING( "Pipeline already initialized!" );
        return true;
    }

    m_renderer = pRenderer;

    auto shaderRepo = CoreAPI::get_assetmanager()->get_repository<Shader>();
    IE_ASSERT( shaderRepo != nullptr );

    // load shaders
    auto vertexShaderAsset = shaderRepo->require_asset( pRenderer->add_shaderformat_fileextension( "SpriteBatch.vert" ) );
    if ( vertexShaderAsset.has_value() == false ) {
        IE_LOG_ERROR( "Vertex Shader not found!" );
        return false;
    }

    auto fragmentShaderAsset = shaderRepo->require_asset( pRenderer->add_shaderformat_fileextension( "TextureXColor.frag" ) );
    if ( fragmentShaderAsset.has_value() == false ) {
        IE_LOG_ERROR( "Fragment Shader not found!" );
        return false;
    }

    AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
    AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

    IE_ASSERT( vertexShader.get()->create_device_ressources( pRenderer, { 0, 0, 1, 1 } ) );
    IE_ASSERT( fragmentShader.get()->create_device_ressources( pRenderer, { 1, 0, 0, 0 } ) );

    // Create the pipeline
    SDL_GPUColorTargetDescription colorTargets[1]     = {};
    colorTargets[0].format                            = SDL_GetGPUSwapchainTextureFormat( m_renderer->get_gpudevice(), m_renderer->get_window()->get_sdlwindow() );
    colorTargets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargets[0].blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    colorTargets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargets[0].blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    colorTargets[0].blend_state.enable_blend          = true;

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo     = {};
    pipelineCreateInfo.vertex_shader                         = vertexShader.get()->get_sdlshader();
    pipelineCreateInfo.fragment_shader                       = fragmentShader.get()->get_sdlshader();
    pipelineCreateInfo.primitive_type                        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineCreateInfo.target_info.color_target_descriptions = colorTargets;
    pipelineCreateInfo.target_info.num_color_targets         = 1;

    m_pipeline = SDL_CreateGPUGraphicsPipeline( m_renderer->get_gpudevice(), &pipelineCreateInfo );
    if ( m_pipeline == nullptr ) {
        IE_LOG_ERROR( "Failed to create pipeline!" );
        return false;
    }

    SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
    tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbufferCreateInfo.size                            = SpriteBatchSizeMax *  sizeof( SpriteVertexUniform );

    m_spriteTransferBuffer = SDL_CreateGPUTransferBuffer( m_renderer->get_gpudevice(), &tbufferCreateInfo );
    if ( m_spriteTransferBuffer == nullptr ) {
        IE_LOG_ERROR( "Failed to create GPUTransferBuffer!" );
        return false;
    }

    m_renderCmd    = std::make_shared<DoubleBufferedCommandQueue<SpriteBatchInfo>>();
    m_spriteAssets = CoreAPI::get_assetmanager()->get_repository<Sprite>();
    m_batches.reserve( 100 );

    m_initialized = true;
    return true;
}

void Sprite2DPipeline::dispatch_copycommands( SDL_GPUCommandBuffer* cmdbuf, SDL_GPUCopyPass* copyPass )
{
    IE_ASSERT( m_renderer != nullptr && m_renderer->get_gpudevice() != nullptr );
    IE_ASSERT( cmdbuf != nullptr && copyPass != nullptr );
    IE_ASSERT( get_commandqueue() != nullptr );
    (void)cmdbuf;

    SpriteVertexUniform* dataPtr = nullptr;
    AssetUID<Sprite>     currentSprite;

    bool       firstBatch   = true;
    BatchData* currentBatch = nullptr;

    for ( Sprite2DPipeline::SpriteBatchInfo* sprite : get_commandqueue()->get_rendercommands() ) {
        // start new batch when changing texture or max batch size is reached (should be sorted by texture at this point)
        if ( currentBatch == nullptr || currentSprite != sprite->texture || currentBatch->count >= SpriteBatchSizeMax ) {
            // unmap and upload previous batch data before changeing to new batch
            if ( firstBatch == false ) {
                SDL_UnmapGPUTransferBuffer( m_renderer->get_gpudevice(), m_spriteTransferBuffer );
                SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_spriteTransferBuffer, .offset = 0 };
                SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( currentBatch->bufferIdx ),
                                                             .offset = 0,
                                                             .size   = static_cast<uint32_t>( currentBatch->count * sizeof( SpriteVertexUniform ) ) };
                SDL_UploadToGPUBuffer( copyPass, &tranferBufferLocation, &bufferRegion, true );
                dataPtr = nullptr;
            }

            currentBatch          = add_batch();
            currentBatch->texture = sprite->texture;
            currentSprite         = sprite->texture;

            dataPtr    = static_cast<SpriteVertexUniform*>( SDL_MapGPUTransferBuffer( m_renderer->get_gpudevice(), m_spriteTransferBuffer, true ) );
            firstBatch = false;
        }

        // add to batch
        dataPtr[currentBatch->count++] = sprite->info;
    }

    // unmap and upload last batch data
    if ( currentBatch && currentBatch->count > 0 ) {
        SDL_UnmapGPUTransferBuffer( m_renderer->get_gpudevice(), m_spriteTransferBuffer );
        SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_spriteTransferBuffer, .offset = 0 };
        SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( currentBatch->bufferIdx ),
                                                     .offset = 0,
                                                     .size   = static_cast<uint32_t>( currentBatch->count * sizeof( SpriteVertexUniform ) ) };
        SDL_UploadToGPUBuffer( copyPass, &tranferBufferLocation, &bufferRegion, true );
    }
}

void Sprite2DPipeline::dispatch_rendercommands( const DXSM::Matrix& viewProjection, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass )
{
    IE_ASSERT( m_renderer != nullptr && m_renderer->get_gpudevice() != nullptr );
    IE_ASSERT( cmdbuf != nullptr && renderPass != nullptr );
    IE_ASSERT( m_spriteAssets.expired() == false );

    SDL_BindGPUGraphicsPipeline( renderPass, m_pipeline );
    SDL_PushGPUVertexUniformData( cmdbuf, 0, &viewProjection, sizeof( DXSM::Matrix ) );

    auto spriteAssets = m_spriteAssets.lock();
    for ( size_t i = 0; i < m_batches.size(); ++i ) {
        if ( m_batches[i].texture.valid() == false ) {
            continue;
        }

        auto gpuBuffer = get_gpubuffer_by_index( m_batches[i].bufferIdx );
        SDL_BindGPUVertexStorageBuffers( renderPass, 0, &gpuBuffer, 1 );

        auto texture = spriteAssets->get_asset( m_batches[i].texture );
        SDL_BindGPUFragmentSamplers( renderPass, 0, &texture->m_textureSamplerBinding, 1 );

        SDL_DrawGPUPrimitives( renderPass, m_batches[i].count * 6, 1, 0, 0 );
    }
    clear_batches();
}

const std::string_view Sprite2DPipeline::get_name() const
{
    return "Sprite2DPipeline";
}

void Sprite2DPipeline::sort_commands()
{
    IE_ASSERT( get_commandqueue() != nullptr );
    auto cmdQueue = get_commandqueue();
    cmdQueue->sort( []( const SpriteBatchInfo* a, const SpriteBatchInfo* b ) {
        if ( a->texture < b->texture )
            return true;

        if ( a->info.z > b->info.z )
            return true;

        return false;
    } );
}

uint32_t Sprite2DPipeline::needs_processing() const
{
    return ( get_commandqueue()->get_rendercommands().size() != 0 ) ? PipelineCommand::Render | PipelineCommand::Copy : 0;
}

Sprite2DPipeline::CommandQueue* Sprite2DPipeline::get_commandqueue() const
{
    return static_pointer_cast<Sprite2DPipeline::CommandQueue>( m_renderCmd ).get();
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
    createInfo.size                    = SpriteBatchSizeMax * sizeof( SpriteVertexUniform );

    buffer = SDL_CreateGPUBuffer( m_renderer->get_gpudevice(), &createInfo );
    if ( buffer == nullptr ) {
        CoreAPI::get_application()->raise_critical_error( std::format( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() ) );
    }
    return m_gpuBuffer_used++;
}

SDL_GPUBuffer* Sprite2DPipeline::get_gpubuffer_by_index( uint32_t index ) const
{
    IE_ASSERT( index < m_gpuBuffer_used );
    return m_gpuBuffer[index];
}

void Sprite2DPipeline::collect( AssetUID<Sprite> spriteUID, float x, float y, float angle, float scale_x, float scale_y, DXSM::Color color, uint16_t layer )
{
    IE_ASSERT( m_initialized );
    auto cmdQueue = get_commandqueue();
    cmdQueue->grow_if_needed();

    SpriteBatchInfo* cmd = cmdQueue->create_entry();
    cmd->texture         = spriteUID;
    cmd->info.x          = x;
    cmd->info.y          = y;
    cmd->info.z          = 1.0f - ( ( layer == 0 ) ? 0.0f : static_cast<float>( layer ) / ( std::numeric_limits<uint16_t>::max )() );
    cmd->info.rotation   = angle;
    cmd->info.scale_w    = scale_x;
    cmd->info.scale_h    = scale_y;
    cmd->info.source     = DXSM::Vector4 { 0.0f, 0.0f, 1.0f, 1.0f };
    cmd->info.color      = color;
}
