#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/graphics/TextureBase.h"
#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/Asset.h"

#include <string_view>

namespace InnoEngine
{
    class Texture2D : public TextureBase, public Asset<Texture2D>
    {
        friend class RenderContext;
        friend class GPURenderer;
        friend class AssetRepository<Texture2D>;

        Texture2D() = default;

    public:
        static auto create( TextureSpecifications specs ) -> std::optional<Ref<Texture2D>>;

        static auto create_from_file( const std::filesystem::path& full_path ) -> std::optional<Ref<Texture2D>>;

        Result load_from_file( const std::filesystem::path& full_path );
        Result load_data( const void* pixels, uint32_t pixel_count, SDL_PixelFormat pixel_format );

    private:
        // Inherited via Asset
        Result load_asset( const std::filesystem::path& full_path ) override;
    };

    using TextureList = std::vector<Ref<Texture2D>>;
}    // namespace InnoEngine
