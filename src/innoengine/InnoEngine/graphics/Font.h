#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/Asset.h"

#include <optional>
#include <string_view>

namespace InnoEngine
{
    class Texture2D;
    struct MSDFData;

    class Font : public Asset<Font>
    {
        friend class RenderContext;
        friend class GPURenderer;
        friend class AssetRepository<Font>;

        Font();

    public:
        virtual ~Font() = default;
        static auto create() -> std::optional<Ref<Font>>;

        Ref<Texture2D> get_atlas_texture() const;
        Ref<MSDFData>     get_msdf_data() const;

        float calculate_screen_pix_range( float FontSize ) const;

        // returns the bounding box of the given text
        // x == left; y == bottom; z == right; w == top
        DXSM::Vector4 get_aabb( uint32_t size, std::string_view text ) const;

    private:
        // Inherited via Asset
        Result load_asset( const std::filesystem::path& full_path ) override;
        void   preload_msdf_ascii_data();

    private:
        Ref<Texture2D> m_AtlasTexture;
        Ref<MSDFData>     m_msdfData;

        bool m_Initialized = false;

        RenderCommandBufferIndexType m_RenderCommandBufferIndex = InvalidRenderCommandBufferIndex;
    };

    using FontList = std::vector<Ref<Font>>;
}    // namespace InnoEngine
