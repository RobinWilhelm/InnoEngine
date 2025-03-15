#include "InnoEngine/iepch.h"
#include "InnoEngine/Font.h"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3_image/SDL_image.h"

#include "InnoEngine/Texture2D.h"


#include "InnoEngine/MSDFData.h"

namespace InnoEngine
{
    Font::Font()
    {
        m_msdfData = std::make_shared<MSDFData>();
    }

    auto Font::create() -> std::optional<Ref<Font>>
    {
        Ref<Font> font   = Ref<Font>( new Font );
        return Ref<Font>();
    }

    Ref<Texture2D> Font::get_atlas_texture() const
    {
        return m_atlasTexture;
    }

    Ref<MSDFData> Font::get_msdf_data() const
    {
        return m_msdfData;
    }

    void Font::render( float x, float y, std::string_view text, DXSM::Color color, uint16_t layer, float scale )
    {
        IE_ASSERT( m_atlasTexture != nullptr && m_frameBufferIndex >= 0 );
        static GPURenderer* renderer = CoreAPI::get_gpurenderer();
        IE_ASSERT( renderer != nullptr );
        renderer->add_text( this, x, y, text, color, layer, scale );
    }

    Result Font::load_asset( const std::filesystem::path& full_path )
    {
        if ( m_initialized == true ) {
            return Result::AlreadyInitialized;
        }

        msdfgen::FreetypeHandle* freetype = msdfgen::initializeFreetype();
        IE_ASSERT( freetype != nullptr );

        msdfgen::FontHandle* font = msdfgen::loadFont( freetype, full_path.string().c_str() );
        IE_ASSERT( font != nullptr );

        m_msdfData->FontGeo = FontGeometry( &m_msdfData->GlyphGeo );

        // Load a set of character glyphs:
        // The second argument can be ignored unless you mix different font sizes in one atlas.
        // In the last argument, you can specify a charset other than ASCII.
        // To load specific glyph indices, use loadGlyphs instead.
            // From imgui_draw.cpp
        struct CharsetRange
        {
            uint32_t Begin, End;
        };

        static const CharsetRange charsetRanges[] =
        {
            { 0x0020, 0x00FF }
        };

        msdf_atlas::Charset charset;
        for (CharsetRange range : charsetRanges)
        {
            for (uint32_t c = range.Begin; c <= range.End; c++)
                charset.add(c);
        }

        m_msdfData->FontGeo.loadCharset( font, 1.0, charset);

        // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
        const double maxCornerAngle = 3.0;
        for ( GlyphGeometry& glyph : m_msdfData->GlyphGeo )
            glyph.edgeColoring( &msdfgen::edgeColoringInkTrap, maxCornerAngle, 0 );

        // TightAtlasPacker class computes the layout of the atlas.
        TightAtlasPacker packer;

        // Set atlas parameters:
        // setDimensions or setDimensionsConstraint to find the best value
        packer.setDimensionsConstraint( DimensionsConstraint::SQUARE );

        // setScale for a fixed size or setMinimumScale to use the largest that fits
        packer.setMinimumScale( 24.0 );

        // setPixelRange or setUnitRange
        packer.setPixelRange( 2.0 );
        packer.setMiterLimit( 1.0 );
        packer.setScale(40);

        // Compute atlas layout - pack glyphs
        packer.pack( m_msdfData->GlyphGeo.data(), static_cast<int>( m_msdfData->GlyphGeo.size() ) );

        // Get final atlas dimensions
        int width = 0, height = 0;
        packer.getDimensions( width, height );

        // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
        ImmediateAtlasGenerator<
            float,                         // pixel type of buffer for individual glyphs depends on generator function
            3,                             // number of atlas color channels
            msdfGenerator,                 // function to generate bitmaps for individual glyphs
            BitmapAtlasStorage<byte, 3>    // class that stores the atlas bitmap
            // For example, a custom atlas storage class that stores it in VRAM can be used.
            >
            generator( width, height );

        // GeneratorAttributes can be modified to change the generator's default settings.
        GeneratorAttributes attributes;
        generator.setAttributes( attributes );
        generator.setThreadCount( 4 );

        // Generate atlas bitmap
        generator.generate( m_msdfData->GlyphGeo.data(), static_cast<int>( m_msdfData->GlyphGeo.size() ) );


        // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
        // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
        auto bmp       = static_cast<msdfgen::BitmapConstRef<byte, 3>>( generator.atlasStorage() );
        m_atlasTexture = Texture2D::create( width, height, TextureFormat::RGBX, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, false).value();
        Result res     = m_atlasTexture->load_data( bmp.pixels, bmp.width * bmp.height, SDL_PIXELFORMAT_RGB24 );

        // Cleanup
        msdfgen::destroyFont( font );
        msdfgen::deinitializeFreetype( freetype );

        if ( IE_SUCCESS( res ) ) {
            IE_LOG_DEBUG( "Loaded font {}", full_path.string() );

/*
            // testing
            SDL_Surface* surface = SDL_CreateSurfaceFrom( width, height, SDL_PixelFormat::SDL_PIXELFORMAT_RGB24, const_cast<void*>( static_cast<const void*>( bmp.pixels ) ), width * 3 );
            SDL_SaveBMP( surface, std::format( "{}.bmp", full_path.filename().string() ).c_str() );
            SDL_DestroySurface( surface );
*/
        }
        return res;
    }
}    // namespace InnoEngine
