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
        friend class GPURenderer;
        friend class AssetRepository<Font>;
        Font();

    public:
        virtual ~Font() = default;
        static auto create() -> std::optional<Ref<Font>>;

        Ref<Texture2D> get_atlas_texture() const;
        Ref<MSDFData>  get_msdf_data() const;

        float calculate_screen_pix_range( float FontSize ) const;

        void render( float x, float y, uint32_t size, std::string_view text, DXSM::Color ForegroundColor = { 1.0f, 1.0f, 1.0f, 1.0f } );

        // returns the bounding box of the given text 
        // x == left; y == bottom; z == right; w == top
        DXSM::Vector4 get_aabb( uint32_t size, std::string_view text ) const;

    private:
        // Inherited via Asset
        Result load_asset( const std::filesystem::path& full_path ) override;
        void   preload_msdf_ascii_data();

    private:
        Ref<Texture2D> m_atlasTexture;
        Ref<MSDFData>  m_msdfData;

        bool m_Initialized = false;

        FrameBufferIndex m_frameBufferIndex = -1;
    };

    using FontList = std::vector<Ref<Font>>;
}    // namespace InnoEngine
