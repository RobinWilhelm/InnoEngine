#include "iepch.h"
#include "Texture2D.h"

#include "SDL3_image/SDL_image.h"

#include "Renderer.h"

namespace InnoEngine
{
    bool Texture2D::load_from_file( const std::filesystem::path& full_path, std::string_view file_name )
    {
        (void)file_name;
        m_surface = IMG_Load( full_path.string().c_str() );
        if ( m_surface == nullptr ) {
            SDL_Log( "%s: %s", std::source_location::current().function_name(), "Could not load image!" );
            return false;
        }

        m_width  = static_cast<uint32_t>( m_surface->w );
        m_height = static_cast<uint32_t>( m_surface->h );
        m_format = m_surface->format;
        return true;
    }

    std::filesystem::path Texture2D::build_path( const std::filesystem::path& folder, std::string_view file_name )
    {
        return folder / file_name;
    }

    Result Texture2D::create_device_ressources( Ref<SDL_GPUDevice> device )
    {
        if ( m_initialized )
            return Result::AlreadyInitialized;

        IE_ASSERT( device != nullptr );
        IE_ASSERT( m_surface != nullptr );

        m_device = device;

        SDL_GPUTextureCreateInfo textureCreateInfo = {};
        textureCreateInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureCreateInfo.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureCreateInfo.width                    = m_width;
        textureCreateInfo.height                   = m_height;
        textureCreateInfo.layer_count_or_depth     = 1;
        textureCreateInfo.num_levels               = 1;

        m_texture                                    = SDL_CreateGPUTexture( device.get(), &textureCreateInfo );
        SDL_GPUTransferBuffer* textureTransferBuffer = nullptr;

        if ( m_texture ) {
            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
            transferBufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size                            = m_height * m_width * 4;
            textureTransferBuffer                                    = SDL_CreateGPUTransferBuffer( device.get(), &transferBufferCreateInfo );

            if ( textureTransferBuffer ) {
                Uint8* textureTransferPtr = static_cast<Uint8*>( SDL_MapGPUTransferBuffer( device.get(), textureTransferBuffer, false ) );

                if ( textureTransferPtr ) {
                    SDL_memcpy( textureTransferPtr, m_surface->pixels, m_width * m_height * 4 );
                    SDL_UnmapGPUTransferBuffer( device.get(), textureTransferBuffer );
                    // Upload the transfer data to the GPU resources
                    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer( device.get() );

                    if ( uploadCmdBuf ) {
                        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass( uploadCmdBuf );

                        if ( copyPass ) {
                            SDL_GPUTextureTransferInfo textureTransferInfo = {};
                            textureTransferInfo.transfer_buffer            = textureTransferBuffer;
                            textureTransferInfo.offset                     = 0; /* Zeroes out the rest */

                            SDL_GPUTextureRegion textureRegion = {};
                            textureRegion.texture              = m_texture;
                            textureRegion.w                    = m_width;
                            textureRegion.h                    = m_height;
                            textureRegion.d                    = 1;

                            SDL_UploadToGPUTexture( copyPass, &textureTransferInfo, &textureRegion, false );
                            SDL_EndGPUCopyPass( copyPass );
                            m_initialized = SDL_SubmitGPUCommandBuffer( uploadCmdBuf );
                        }
                    }
                }
            }
        }

        SDL_DestroySurface( m_surface );
        m_surface = nullptr;
        if ( textureTransferBuffer )
            SDL_ReleaseGPUTransferBuffer( device.get(), textureTransferBuffer );
        return m_initialized ? Result::Success : Result::InitializationError;
    }

    void Texture2D::release_device_ressources()
    {
        if ( m_surface ) {
            SDL_DestroySurface( m_surface );
            m_surface = nullptr;
        }

        if ( m_texture ) {
            SDL_ReleaseGPUTexture( m_device.get(), m_texture );
            m_texture = nullptr;
        }

        m_device.reset();
        m_initialized = false;
    }

    SDL_PixelFormat Texture2D::get_format() const
    {
        return SDL_PixelFormat();
    }

    int Texture2D::width() const
    {
        return 0;
    }

    int Texture2D::height() const
    {
        return 0;
    }

    SDL_GPUTexture* Texture2D::get_sdltexture() const
    {
        return m_texture;
    }
}    // namespace InnoEngine
