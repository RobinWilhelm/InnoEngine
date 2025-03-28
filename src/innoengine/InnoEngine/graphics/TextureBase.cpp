#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/TextureBase.h"

#include "InnoEngine/utility/Log.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/Renderer.h"

namespace InnoEngine
{
    TextureBase::TextureBase()
    {
        m_Device = CoreAPI::get_gpurenderer()->get_gpudevice();
    }

    Result TextureBase::create_gpu_texture( const TextureSpecifications& specs )
    {
        IE_ASSERT( m_Device != nullptr );

        m_Format = convert_to_sdltextureformat( specs.Format );


        SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        if(specs.EnableMipmap || specs.RenderTarget)
            usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

        bool supported = SDL_GPUTextureSupportsFormat( m_Device, m_Format, SDL_GPU_TEXTURETYPE_2D, usage );
        if ( supported == false ) {
            return Result::InvalidParameters;
        }

        m_TexelBlockSize = SDL_GPUTextureFormatTexelBlockSize( m_Format );

        if ( specs.EnableMipmap )
            m_MipLevels = calculate_miplevels( specs.Width, specs.Height );

        SDL_GPUTextureCreateInfo textureCreateInfo = {};
        textureCreateInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
        textureCreateInfo.format                   = m_Format;
        textureCreateInfo.usage                    = usage;
        textureCreateInfo.width                    = specs.Width;
        textureCreateInfo.height                   = specs.Height;
        textureCreateInfo.layer_count_or_depth     = 1;
        textureCreateInfo.num_levels               = m_MipLevels;

        m_Texture = SDL_CreateGPUTexture( m_Device, &textureCreateInfo );
        if ( m_Texture == nullptr ) {
            IE_LOG_ERROR( "SDL_CreateGPUTexture failed: {}", SDL_GetError() );
            return Result::Fail;
        }

        m_Specs = specs;
        return Result::Success;
    }

    TextureBase::~TextureBase()
    {
        if (m_Texture)
        {
            SDL_ReleaseGPUTexture(m_Device, m_Texture);
            m_Texture = nullptr;
        }
    }

    SDL_GPUTexture* TextureBase::get_sdltexture() const
    {
        return m_Texture;
    }

    const TextureSpecifications& TextureBase::get_specs() const
    {
        return m_Specs;
    }

    uint32_t TextureBase::calculate_miplevels( uint32_t width, uint32_t height )
    {
        uint32_t mip_levels = 0;
        uint32_t size       = max( width, height );
        while ( size > 1 ) {
            size /= 2;
            ++mip_levels;
        }
        return mip_levels;
    }

    SDL_GPUTextureFormat TextureBase::convert_to_sdltextureformat( TextureFormat texture_format )
    {
        switch ( texture_format ) {
        case TextureFormat::RGBA:
        case TextureFormat::RGBX:
            return SDL_GPUTextureFormat::SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        default:
            IE_LOG_ERROR( "Invalid texture format \"{}\"", static_cast<uint32_t>( texture_format ) );
            return SDL_GPUTextureFormat::SDL_GPU_TEXTUREFORMAT_INVALID;
        }
    }

    TextureFormat TextureBase::convert_sdlpixelformat( SDL_PixelFormat pixel_format )
    {
        switch ( pixel_format ) {
        case SDL_PIXELFORMAT_RGB24:
            return TextureFormat::RGBX;
        case SDL_PIXELFORMAT_RGBA32:
            return TextureFormat::RGBA;
        default:
            return TextureFormat::Invalid;
        }
    }

    Result TextureBase::copy_pixels_from_surface( void* texture_data, SDL_GPUTextureFormat destination_format, const void* surface_data, SDL_PixelFormat surface_format, uint32_t pixel_count )
    {
        std::byte*       texture_bytes = static_cast<std::byte*>( texture_data );
        const std::byte* surface_bytes = static_cast<const std::byte*>( surface_data );

        switch ( surface_format ) {
        case SDL_PIXELFORMAT_RGB24:
            switch ( destination_format ) {
            case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
                for ( size_t i = 0; i < pixel_count; ++i ) {
                    texture_bytes[ i * 4 ]     = surface_bytes[ i * 3 ];
                    texture_bytes[ i * 4 + 1 ] = surface_bytes[ i * 3 + 1 ];
                    texture_bytes[ i * 4 + 2 ] = surface_bytes[ i * 3 + 2 ];                       
                    texture_bytes[ i * 4 + 3 ] = static_cast<std::byte>( 255 );
                }
                return Result::Success;
            }
            break;
        case SDL_PIXELFORMAT_RGBA32:
            switch ( destination_format ) {
            case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
                SDL_memcpy( texture_bytes, surface_bytes, pixel_count * 4 );
                return Result::Success;
            }
            break;
        }
        return Result::Fail;
    }    // namespace InnoEngine
}    // namespace InnoEngine
