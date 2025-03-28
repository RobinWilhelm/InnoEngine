#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "SDL3_image/SDL_image.h"

#include "Renderer.h"

namespace InnoEngine
{
    auto Texture2D::create( TextureSpecifications specs ) -> std::optional<Ref<Texture2D>>
    {
        Ref<Texture2D> texture = Ref<Texture2D>( new Texture2D() );

        if ( IE_SUCCESS( texture->create_gpu_texture( specs ) ) ) {
            return texture;
        }
        return std::nullopt;
    }

    auto Texture2D::create_from_file( const std::filesystem::path& full_path ) -> std::optional<Ref<Texture2D>>
    {
        Ref<Texture2D> texture = Ref<Texture2D>( new Texture2D() );
        if ( IE_SUCCESS( texture->load_from_file( full_path ) ) ) {
            return texture;
        }
        return std::nullopt;
    }

    Result Texture2D::load_asset( const std::filesystem::path& full_path )
    {
        return load_from_file( full_path );
    }

    Result Texture2D::load_from_file( const std::filesystem::path& full_path )
    {
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( m_Texture == nullptr );

        SDL_Surface* surface = IMG_Load( full_path.string().c_str() );
        if ( surface == nullptr ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed at IMG_Load: {}", full_path.filename().string(), SDL_GetError() );
            return Result::Fail;
        }

        TextureFormat format = convert_sdlpixelformat( surface->format );
        if ( format == TextureFormat::Invalid ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed: {}", full_path.filename().string(), "Unsupported texture format" );
            return Result::Fail;
        }

        TextureSpecifications specs = {};
        specs.Width                 = surface->w;
        specs.Height                = surface->h;
        specs.Format                = format;
        specs.EnableMipmap          = true;

        Result res = create_gpu_texture( specs );
        if ( IE_SUCCESS( res ) ) {
            res = load_data( surface->pixels, surface->w * surface->h, surface->format );
        }
        SDL_DestroySurface( surface );

        if ( res == Result::Success )
            IE_LOG_DEBUG( "Loaded texture \"{}\"", full_path.filename().string() );

        return res;
    }

    Result Texture2D::load_data( const void* pixels, uint32_t pixel_count, SDL_PixelFormat pixel_format )
    {
        IE_ASSERT( pixels != nullptr );
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( m_Texture != nullptr );

        if ( convert_sdlpixelformat( pixel_format ) == TextureFormat::Invalid ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed: {}", get_path().filename().string(), "Pixel format not supported" );
            return Result::Fail;
        }

        const SDL_PixelFormatDetails* pixelFormat = SDL_GetPixelFormatDetails( pixel_format );
        if ( pixelFormat == nullptr ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed at SDL_GetPixelFormatDetails: {}", get_path().filename().string(), SDL_GetError() );
            return Result::Fail;
        }

        uint32_t               size                  = pixel_count * m_TexelBlockSize;
        SDL_GPUTransferBuffer* textureTransferBuffer = nullptr;

        bool success = false;

        if ( m_Texture ) {
            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
            transferBufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size                            = size;
            textureTransferBuffer                                    = SDL_CreateGPUTransferBuffer( m_Device, &transferBufferCreateInfo );

            if ( textureTransferBuffer ) {
                Uint8* textureTransferPtr = static_cast<Uint8*>( SDL_MapGPUTransferBuffer( m_Device, textureTransferBuffer, false ) );

                if ( textureTransferPtr ) {
                    copy_pixels_from_surface( textureTransferPtr, m_Format, pixels, pixel_format, pixel_count );

                    SDL_UnmapGPUTransferBuffer( m_Device, textureTransferBuffer );
                    // Upload the transfer data to the GPU resources
                    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer( m_Device );

                    if ( uploadCmdBuf ) {
                        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass( uploadCmdBuf );

                        if ( copyPass ) {
                            SDL_GPUTextureTransferInfo textureTransferInfo = {};
                            textureTransferInfo.transfer_buffer            = textureTransferBuffer;
                            textureTransferInfo.offset                     = 0; /* Zeroes out the rest */
                            textureTransferInfo.pixels_per_row             = m_Specs.Width;

                            SDL_GPUTextureRegion textureRegion = {};
                            textureRegion.texture              = m_Texture;
                            // textureRegion.mip_level            = 1;
                            textureRegion.w                    = m_Specs.Width;
                            textureRegion.h                    = m_Specs.Height;
                            textureRegion.d                    = 1;

                            SDL_UploadToGPUTexture( copyPass, &textureTransferInfo, &textureRegion, false );
                            SDL_EndGPUCopyPass( copyPass );

                            if ( m_MipLevels > 1 )
                                SDL_GenerateMipmapsForGPUTexture( uploadCmdBuf, m_Texture );

                            success = SDL_SubmitGPUCommandBuffer( uploadCmdBuf );
                        }
                    }
                }
            }
        }
        if ( textureTransferBuffer ) {
            SDL_ReleaseGPUTransferBuffer( m_Device, textureTransferBuffer );
        }

        return success ? Result::Success : Result::InitializationError;
    }
}    // namespace InnoEngine
