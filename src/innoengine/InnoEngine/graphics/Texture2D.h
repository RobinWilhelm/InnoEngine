#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/Asset.h"
#include "InnoEngine/graphics/Renderer.h"

#include <string_view>

namespace InnoEngine
{
    enum class TextureFormat
    {
        Invalid = 0,
        RGBA,
        RGBX
    };

    class Texture2D : public Asset<Texture2D>
    {
        friend class RenderContext;
        friend class GPURenderer;
        friend class AssetRepository<Texture2D>;

        Texture2D() :
            m_Device( CoreAPI::get_gpurenderer()->get_gpudevice() ) { };

    public:
        virtual ~Texture2D();

        static auto create( uint32_t width, uint32_t height,
                            TextureFormat            format        = TextureFormat::RGBA,
                            SDL_GPUTextureUsageFlags usage         = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                            bool                     enable_mipmap = true ) -> std::optional<Ref<Texture2D>>;

        static auto create_from_file( const std::filesystem::path& full_path ) -> std::optional<Ref<Texture2D>>;

        static uint32_t calculate_miplevels( uint32_t width, uint32_t height );

        Result load_from_file( const std::filesystem::path& full_path );
        Result load_data( const void* pixels, uint32_t pixel_count, SDL_PixelFormat pixel_format );

        // void save_to_file(const std::filesystem::path& full_path);

        SDL_PixelFormat get_format() const;
        int             width() const;
        int             height() const;

        SDL_GPUTexture* get_sdltexture() const;

        static bool pixelformat_supported( SDL_PixelFormat pixel_format );

    private:
        // Inherited via Asset
        Result load_asset( const std::filesystem::path& full_path ) override;
        Result create_gpu_texture( uint32_t width, uint32_t height, TextureFormat format,
                                   SDL_GPUTextureUsageFlags usage, bool enable_mipmap );

        void                        map_pixels( void* texture, const void* surface, uint32_t source_count, SDL_PixelFormat source_format ) const;
        static SDL_GPUTextureFormat textureformat_to_sdl_gpu_textureformat( TextureFormat format );
        static TextureFormat        sdl_pixelformat_to_textureformat( SDL_PixelFormat format );

    private:
        GPUDeviceRef    m_Device  = nullptr;
        SDL_GPUTexture* m_texture = nullptr;

        int           m_width     = 0;
        int           m_height    = 0;
        uint32_t      m_miplevels = 0;
        TextureFormat m_format    = TextureFormat::Invalid;

        uint32_t m_texelBlockSize = 0;

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
    };

    using TextureList = std::vector<Ref<Texture2D>>;
}    // namespace InnoEngine
