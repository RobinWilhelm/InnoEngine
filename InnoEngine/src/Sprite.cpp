#include "iepch.h"
#include "Sprite.h"

#include "SDL3_image/SDL_image.h"

#include "Asset.h"
#include "Renderer.h"
#include "Sprite2DPipeline.h"

Sprite::~Sprite()
{
    release_device_ressources();
}

bool Sprite::load_from_file( std::filesystem::path fullPath )
{
    // Load the texture
    m_imageData = IMG_Load( fullPath.string().c_str() );
    if ( m_imageData == nullptr ) {
        SDL_Log( "%s: %s", std::source_location::current().function_name(), "Could not load image!" );
        return false;
    }

    m_width  = static_cast<uint32_t>( m_imageData->w );
    m_height = static_cast<uint32_t>( m_imageData->h );
    m_format = m_imageData->format;
    return true;
}

bool Sprite::create_device_ressources( GPURenderer* pRenderer )
{
    if ( m_ready )
        return true;

    if ( m_imageData == nullptr )
        return false;

    m_renderer             = pRenderer;
    SDL_GPUDevice* pDevice = m_renderer->get_gpudevice();

    SDL_GPUBufferCreateInfo vertexbufferCreateInfo = {};
    vertexbufferCreateInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexbufferCreateInfo.size                    = sizeof( Sprite2DPipeline::Vertex ) * 4;

    m_vertexBuffer = SDL_CreateGPUBuffer( pDevice, &vertexbufferCreateInfo );
    if ( m_vertexBuffer == nullptr ) {
        return false;
    }

    SDL_GPUBufferCreateInfo indexbufferCreateInfo = {};
    indexbufferCreateInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    indexbufferCreateInfo.size                    = sizeof( Uint16 ) * 6;

    m_indexBuffer = SDL_CreateGPUBuffer( pDevice, &indexbufferCreateInfo );
    if ( m_indexBuffer == nullptr ) {
        release_device_ressources();
        return false;
    }

    SDL_GPUTextureCreateInfo textureCreateInfo = {};
    textureCreateInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
    textureCreateInfo.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureCreateInfo.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    textureCreateInfo.width                    = m_width;
    textureCreateInfo.height                   = m_height;
    textureCreateInfo.layer_count_or_depth     = 1;
    textureCreateInfo.num_levels               = 1;

    m_texture = SDL_CreateGPUTexture( pDevice, &textureCreateInfo );
    if ( m_texture == nullptr ) {
        release_device_ressources();
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.min_filter               = SDL_GPU_FILTER_NEAREST;
    samplerCreateInfo.mag_filter               = SDL_GPU_FILTER_NEAREST;
    samplerCreateInfo.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerCreateInfo.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    m_sampler = SDL_CreateGPUSampler( pDevice, &samplerCreateInfo );
    if ( m_sampler == nullptr ) {
        release_device_ressources();
        return false;
    }

    // Set up buffer data
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
    transferBufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferCreateInfo.size                            = ( sizeof( Sprite2DPipeline::Vertex ) * 4 ) + ( sizeof( Uint16 ) * 6 );

    SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer( pDevice, &transferBufferCreateInfo );
    if ( bufferTransferBuffer == nullptr ) {
        release_device_ressources();
        return false;
    }

    Sprite2DPipeline::Vertex* transferData = static_cast<Sprite2DPipeline::Vertex*>( SDL_MapGPUTransferBuffer( pDevice, bufferTransferBuffer, false ) );
    if ( transferData == nullptr ) {
        SDL_ReleaseGPUTransferBuffer( pDevice, bufferTransferBuffer );
        release_device_ressources();
        return false;
    }

    float width  = static_cast<float>( m_width );
    float height = static_cast<float>( m_height );

    transferData[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    transferData[1] = { width, 0.0f, 0.0f, 1.0f, 0.0f };
    transferData[2] = { width, height, 0.0f, 1.0f, 1.0f };
    transferData[3] = { 0.0f, height, 0.0f, 0.0f, 1.0f };

    Uint16* indexData = (Uint16*)&transferData[4];
    indexData[0]      = 0;
    indexData[1]      = 1;
    indexData[2]      = 2;
    indexData[3]      = 0;
    indexData[4]      = 2;
    indexData[5]      = 3;

    SDL_UnmapGPUTransferBuffer( pDevice, bufferTransferBuffer );

    // Set up texture data
    transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferCreateInfo.size  = m_height * m_width * 4;

    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer( pDevice, &transferBufferCreateInfo );
    if ( textureTransferBuffer == nullptr ) {
        SDL_ReleaseGPUTransferBuffer( pDevice, bufferTransferBuffer );
        release_device_ressources();
        return false;
    }

    Uint8* textureTransferPtr = static_cast<Uint8*>( SDL_MapGPUTransferBuffer( pDevice, textureTransferBuffer, false ) );
    if ( textureTransferPtr == nullptr ) {
        SDL_ReleaseGPUTransferBuffer( pDevice, textureTransferBuffer );
        SDL_ReleaseGPUTransferBuffer( pDevice, bufferTransferBuffer );
        release_device_ressources();
        return false;
    }

    SDL_memcpy( textureTransferPtr, m_imageData->pixels, m_width * m_height * 4 );
    SDL_UnmapGPUTransferBuffer( pDevice, textureTransferBuffer );

    // Upload the transfer data to the GPU resources
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer( pDevice );
    if ( uploadCmdBuf == nullptr ) {
        SDL_ReleaseGPUTransferBuffer( pDevice, textureTransferBuffer );
        SDL_ReleaseGPUTransferBuffer( pDevice, bufferTransferBuffer );
        release_device_ressources();
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass( uploadCmdBuf );

    SDL_GPUTransferBufferLocation transferBufferLocation = {};
    transferBufferLocation.transfer_buffer               = bufferTransferBuffer;
    transferBufferLocation.offset                        = 0;

    SDL_GPUBufferRegion bufferRegion = {};
    bufferRegion.buffer              = m_vertexBuffer;
    bufferRegion.offset              = 0;
    bufferRegion.size                = sizeof( Sprite2DPipeline::Vertex ) * 4;

    SDL_UploadToGPUBuffer( copyPass, &transferBufferLocation, &bufferRegion, false );

    transferBufferLocation = { .transfer_buffer = bufferTransferBuffer, .offset = sizeof( Sprite2DPipeline::Vertex ) * 4 };
    bufferRegion.buffer    = m_indexBuffer;
    bufferRegion.offset    = 0;
    bufferRegion.size      = sizeof( Uint16 ) * 6;

    SDL_UploadToGPUBuffer( copyPass, &transferBufferLocation, &bufferRegion, false );

    SDL_GPUTextureTransferInfo textureTransferInfo = {};
    textureTransferInfo.transfer_buffer            = textureTransferBuffer;
    textureTransferInfo.offset                     = 0; /* Zeroes out the rest */

    SDL_GPUTextureRegion textureRegion = {};
    textureRegion.texture              = m_texture;
    textureRegion.w                    = m_width;
    textureRegion.h                    = m_height;
    textureRegion.d                    = 1;

    SDL_UploadToGPUTexture( copyPass, &textureTransferInfo, &textureRegion, false );

    SDL_DestroySurface( m_imageData );
    SDL_EndGPUCopyPass( copyPass );
    SDL_SubmitGPUCommandBuffer( uploadCmdBuf );
    SDL_ReleaseGPUTransferBuffer( pDevice, bufferTransferBuffer );
    SDL_ReleaseGPUTransferBuffer( pDevice, textureTransferBuffer );

    m_vertexBufferBinding   = { .buffer = m_vertexBuffer, .offset = 0 };
    m_indexBufferBinding    = { .buffer = m_indexBuffer, .offset = 0 };
    m_textureSamplerBinding = { .texture = m_texture, .sampler = m_sampler };

    m_imageData = nullptr;

    auto piplineOpt = m_renderer->require_pipeline<Sprite2DPipeline>();
    if ( piplineOpt.has_value() == false )
        return false;

    m_pipeline = piplineOpt.value();

    m_ready = true;
    return true;
}

void Sprite::release_device_ressources()
{
    if ( m_vertexBuffer ) {
        SDL_ReleaseGPUBuffer( m_renderer->get_gpudevice(), m_vertexBuffer );
        m_vertexBuffer = nullptr;
    }
    if ( m_indexBuffer ) {
        SDL_ReleaseGPUBuffer( m_renderer->get_gpudevice(), m_indexBuffer );
        m_indexBuffer = nullptr;
    }
    if ( m_texture ) {
        SDL_ReleaseGPUTexture( m_renderer->get_gpudevice(), m_texture );
        m_texture = nullptr;
    }
    if ( m_sampler ) {
        SDL_ReleaseGPUSampler( m_renderer->get_gpudevice(), m_sampler );
        m_sampler = nullptr;
    }
    m_ready = false;
}

SDL_PixelFormat Sprite::get_format() const
{
    return m_format;
}

uint32_t Sprite::get_width() const
{
    return m_width;
}

uint32_t Sprite::get_height() const
{
    return m_height;
}

SDL_GPUTexture* Sprite::get_sdltexture()
{
    return m_texture;
}

SDL_GPUBuffer* Sprite::get_sdlvertexbuffer()
{
    return m_vertexBuffer;
}

SDL_GPUBuffer* Sprite::get_sdlindexbuffer()
{
    return m_indexBuffer;
}

void Sprite::render( float x, float y, DXSM::Color color, uint16_t layer )
{
    render( x, y, 0.0f, 1.0f, color, layer );
}

void Sprite::render( float x, float y, float angle, float scale, DXSM::Color color, uint16_t layer )
{
    m_pipeline->collect( get_uid(), x, y, angle, scale * get_width(), scale * get_height(), color, layer );
}
