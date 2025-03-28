#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

namespace InnoEngine
{
    struct TextureSpecifications
    {
        uint32_t      Width;
        uint32_t      Height;
        TextureFormat Format;
        bool          EnableMipmap;
        bool          RenderTarget;
    };

    class TextureBase
    {
    protected:
        TextureBase();
        Result create_gpu_texture( const TextureSpecifications& specs );

    public:
        virtual ~TextureBase();

        SDL_GPUTexture*              get_sdltexture() const;
        const TextureSpecifications& get_specs() const;

        static uint32_t             calculate_miplevels( uint32_t width, uint32_t height );
        static SDL_GPUTextureFormat convert_to_sdltextureformat( TextureFormat texture_format );
        static TextureFormat        convert_sdlpixelformat( SDL_PixelFormat pixel_format );
        static Result               copy_pixels_from_surface( void* texture_data, SDL_GPUTextureFormat destination_format, const void* surface_data, SDL_PixelFormat surface_format, uint32_t pixel_count );

    protected:
        GPUDeviceRef         m_Device  = nullptr;
        SDL_GPUTexture*      m_Texture = nullptr;
        SDL_GPUTextureFormat m_Format  = {};

        TextureSpecifications m_Specs = {};

        uint32_t m_MipLevels      = 1;
        uint32_t m_TexelBlockSize = 0;

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
    };
}    // namespace InnoEngine
