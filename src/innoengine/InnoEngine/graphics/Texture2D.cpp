#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Texture2D.h"

#include "SDL3_image/SDL_image.h"

#include "Renderer.h"

namespace InnoEngine
{
    Texture2D::~Texture2D()
    {
        if ( m_texture ) {
            SDL_ReleaseGPUTexture( m_device, m_texture );
            m_texture = nullptr;
        }
    }

    auto Texture2D::create( uint32_t width, uint32_t height, TextureFormat format, SDL_GPUTextureUsageFlags usage, bool enable_mipmap ) -> std::optional<Ref<Texture2D>>
    {
        Ref<Texture2D> texture = Ref<Texture2D>( new Texture2D() );

        if ( IE_SUCCESS( texture->create_gpu_texture( width, height, format, usage, enable_mipmap ) ) ) {
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
        IE_ASSERT( m_device != nullptr );
        IE_ASSERT( m_texture == nullptr );

        SDL_Surface* surface = IMG_Load( full_path.string().c_str() );
        if ( surface == nullptr ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed at IMG_Load: {}", full_path.filename().string(), SDL_GetError() );
            return Result::Fail;
        }

        TextureFormat format = sdl_pixelformat_to_textureformat( surface->format );
        if ( format == TextureFormat::Invalid ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed: {}", full_path.filename().string(), "Unsupported texture format" );
            return Result::Fail;
        }

        Result res = create_gpu_texture( surface->w, surface->h, format, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, true );
        if ( IE_SUCCESS( res ) ) {

            res = load_data( surface->pixels, surface->w * surface->h, surface->format );
        }
        SDL_DestroySurface( surface );

        if ( res == Result::Success )
            IE_LOG_DEBUG( "Loaded texture \"{}\"", full_path.filename().string() );

        return res;
    }

    Result Texture2D::create_gpu_texture( uint32_t width, uint32_t height, TextureFormat format, SDL_GPUTextureUsageFlags usage, bool enable_mipmap )
    {
        IE_ASSERT( m_device != nullptr );

        SDL_GPUTextureFormat gpu_texture_format = textureformat_to_sdl_gpu_textureformat( format );

        bool supported = SDL_GPUTextureSupportsFormat( m_device, gpu_texture_format, SDL_GPU_TEXTURETYPE_2D, usage );
        if ( supported == false ) {
            return Result::InvalidParameters;
        }

        m_texelBlockSize = SDL_GPUTextureFormatTexelBlockSize( gpu_texture_format );

        m_width  = width;
        m_height = height;
        m_format = format;

        m_miplevels = 1;
        if ( enable_mipmap )
            m_miplevels = Texture2D::calculate_miplevels( width, height );

        SDL_GPUTextureCreateInfo textureCreateInfo = {};
        textureCreateInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format                   = gpu_texture_format;
        textureCreateInfo.usage                    = usage;
        textureCreateInfo.width                    = width;
        textureCreateInfo.height                   = height;
        textureCreateInfo.layer_count_or_depth     = 1;
        textureCreateInfo.num_levels               = m_miplevels;

        m_texture = SDL_CreateGPUTexture( m_device, &textureCreateInfo );
        if ( m_texture == nullptr ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed at SDL_CreateGPUTexture: {}", get_path().filename().string(), SDL_GetError() );
            return Result::Fail;
        }

        return Result::Success;
    }

    void Texture2D::map_pixels( void* texture, const void* surface, uint32_t pixel_count, SDL_PixelFormat pixel_format ) const
    {
        std::byte*       texture_bytes = static_cast<std::byte*>( texture );
        const std::byte* surface_bytes = static_cast<const std::byte*>( surface );

        SDL_GPUTextureFormat target_format = textureformat_to_sdl_gpu_textureformat( m_format );

        switch ( pixel_format ) {
        case SDL_PIXELFORMAT_RGB24:
            switch ( target_format ) {
            case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
                for ( size_t i = 0; i < pixel_count; ++i ) {
                    texture_bytes[ i * 4 ]     = surface_bytes[ i * 3 ];
                    texture_bytes[ i * 4 + 1 ] = surface_bytes[ i * 3 + 1 ];
                    texture_bytes[ i * 4 + 2 ] = surface_bytes[ i * 3 + 2 ];
                    texture_bytes[ i * 4 + 3 ] = static_cast<std::byte>( 255 );
                }
                break;
            }
            break;
        case SDL_PIXELFORMAT_RGBA32:
            switch ( target_format ) {
            case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
                SDL_memcpy( texture, surface_bytes, pixel_count * 4 );
                break;
            }
            break;
        }
    }

    SDL_GPUTextureFormat Texture2D::textureformat_to_sdl_gpu_textureformat( TextureFormat format )
    {
        switch ( format ) {
        case TextureFormat::RGBA:
        case TextureFormat::RGBX:
            return SDL_GPUTextureFormat::SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        default:
            IE_LOG_ERROR( "Invalid texture format \"{}\"", static_cast<uint32_t>( format ) );
            return SDL_GPUTextureFormat::SDL_GPU_TEXTUREFORMAT_INVALID;
        }
    }

    TextureFormat Texture2D::sdl_pixelformat_to_textureformat( SDL_PixelFormat format )
    {
        switch ( format ) {
        case SDL_PIXELFORMAT_RGB24:
            return TextureFormat::RGBX;
        case SDL_PIXELFORMAT_RGBA32:
            return TextureFormat::RGBA;
        default:
            return TextureFormat::Invalid;
        }
    }

    Result Texture2D::load_data( const void* pixels, uint32_t pixel_count, SDL_PixelFormat pixel_format )
    {
        IE_ASSERT( pixels != nullptr );
        IE_ASSERT( m_device != nullptr );
        IE_ASSERT( m_texture != nullptr );

        if ( pixelformat_supported( pixel_format ) == false ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed: {}", get_path().filename().string(), "Pixel format not supported" );
            return Result::Fail;
        }

        const SDL_PixelFormatDetails* pixelFormat = SDL_GetPixelFormatDetails( pixel_format );
        if ( pixelFormat == nullptr ) {
            IE_LOG_ERROR( "Loading texture \"{}\" failed at SDL_GetPixelFormatDetails: {}", get_path().filename().string(), SDL_GetError() );
            return Result::Fail;
        }

        uint32_t               size                  = pixel_count * m_texelBlockSize;
        SDL_GPUTransferBuffer* textureTransferBuffer = nullptr;

        bool success = false;

        if ( m_texture ) {
            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
            transferBufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size                            = size;
            textureTransferBuffer                                    = SDL_CreateGPUTransferBuffer( m_device, &transferBufferCreateInfo );

            if ( textureTransferBuffer ) {
                Uint8* textureTransferPtr = static_cast<Uint8*>( SDL_MapGPUTransferBuffer( m_device, textureTransferBuffer, false ) );

                if ( textureTransferPtr ) {

                    (void)pixels;
                    map_pixels( textureTransferPtr, pixels, pixel_count, pixel_format );

                    SDL_UnmapGPUTransferBuffer( m_device, textureTransferBuffer );
                    // Upload the transfer data to the GPU resources
                    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer( m_device );

                    if ( uploadCmdBuf ) {
                        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass( uploadCmdBuf );

                        if ( copyPass ) {
                            SDL_GPUTextureTransferInfo textureTransferInfo = {};
                            textureTransferInfo.transfer_buffer            = textureTransferBuffer;
                            textureTransferInfo.offset                     = 0; /* Zeroes out the rest */
                            textureTransferInfo.pixels_per_row             = m_width;

                            SDL_GPUTextureRegion textureRegion = {};
                            textureRegion.texture              = m_texture;
                            // textureRegion.mip_level            = 1;
                            textureRegion.w                    = m_width;
                            textureRegion.h                    = m_height;
                            textureRegion.d                    = 1;

                            SDL_UploadToGPUTexture( copyPass, &textureTransferInfo, &textureRegion, false );
                            SDL_EndGPUCopyPass( copyPass );

                            if ( m_miplevels > 1 )
                                SDL_GenerateMipmapsForGPUTexture( uploadCmdBuf, m_texture );

                            success = SDL_SubmitGPUCommandBuffer( uploadCmdBuf );
                        }
                    }
                }
            }
        }
        if ( textureTransferBuffer ) {
            SDL_ReleaseGPUTransferBuffer( m_device, textureTransferBuffer );
        }

        return success ? Result::Success : Result::InitializationError;
    }

    SDL_PixelFormat Texture2D::get_format() const
    {
        return SDL_PixelFormat();
    }

    int Texture2D::width() const
    {
        return m_width;
    }

    int Texture2D::height() const
    {
        return m_height;
    }

    SDL_GPUTexture* Texture2D::get_sdltexture() const
    {
        return m_texture;
    }

    bool Texture2D::pixelformat_supported( SDL_PixelFormat pixel_format )
    {
        return sdl_pixelformat_to_textureformat( pixel_format ) != TextureFormat::Invalid;
    }

    uint32_t Texture2D::calculate_miplevels( uint32_t width, uint32_t height )
    {
        uint32_t mip_levels = 0;
        uint32_t size       = max( width, height );
        while ( size > 1 ) {
            size /= 2;
            ++mip_levels;
        }
        return mip_levels;
    }
}    // namespace InnoEngine
