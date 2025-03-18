#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Font.h"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3_image/SDL_image.h"

#include "InnoEngine/graphics/Texture2D.h"

#include "InnoEngine/graphics/MSDFData.h"

namespace InnoEngine
{
    Font::Font()
    {
        m_msdfData = std::make_shared<MSDFData>();
    }

    auto Font::create() -> std::optional<Ref<Font>>
    {
        Ref<Font> font = Ref<Font>( new Font );
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

    float Font::calculate_screen_pix_range( float FontSize ) const
    {
        IE_ASSERT( m_msdfData != nullptr );
        return FontSize / m_msdfData->Scale * m_msdfData->Range;
    }

    void Font::render( float x, float y, uint32_t size, std::string_view text, DXSM::Color ForegroundColor)
    {
        IE_ASSERT( m_atlasTexture != nullptr && m_frameBufferIndex >= 0 );
        static GPURenderer* renderer = CoreAPI::get_gpurenderer();
        IE_ASSERT( renderer != nullptr );
        renderer->add_text( this, { x, y }, size, text, ForegroundColor );
    }

    Result Font::load_asset( const std::filesystem::path& full_path )
    {
        if ( m_Initialized == true ) {
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

        msdf_atlas::Charset charset;
        for ( unsigned char c = 0x20; c < 0xFF; ++c ) {
            charset.add( c );
        }

        m_msdfData->FontGeo.loadCharset( font, 1.0, charset );

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
        packer.setPixelRange( m_msdfData->Range );
        packer.setMiterLimit( 1.0 );
        packer.setScale( m_msdfData->Scale );

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
        generator.setThreadCount( 8 );

        // Generate atlas bitmap
        generator.generate( m_msdfData->GlyphGeo.data(), static_cast<int>( m_msdfData->GlyphGeo.size() ) );

        // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
        // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
        auto bmp       = static_cast<msdfgen::BitmapConstRef<byte, 3>>( generator.atlasStorage() );
        m_atlasTexture = Texture2D::create( width, height, TextureFormat::RGBX, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, false ).value();
        Result res     = m_atlasTexture->load_data( bmp.pixels, bmp.width * bmp.height, SDL_PIXELFORMAT_RGB24 );

        // Cleanup
        msdfgen::destroyFont( font );
        msdfgen::deinitializeFreetype( freetype );

        if ( IE_SUCCESS( res ) ) {
            IE_LOG_DEBUG( "Loaded font {}", full_path.string() );
            m_Initialized = true;

            if ( m_msdfData->IsASCII ) {
                preload_msdf_ascii_data();
            }

            /*
            // testing
            SDL_Surface* surface = SDL_CreateSurfaceFrom( width, height, SDL_PixelFormat::SDL_PIXELFORMAT_RGB24, const_cast<void*>( static_cast<const void*>( bmp.pixels ) ), width * 3 );
            SDL_SaveBMP( surface, std::format( "{}.bmp", full_path.filename().string() ).c_str() );
            SDL_DestroySurface( surface );
            */
        }
        return res;
    }

    void Font::preload_msdf_ascii_data()
    {
        IE_ASSERT( m_msdfData != nullptr );

        for ( unsigned char c1 = 0; c1 < 255; ++c1 ) {
            const msdf_atlas::GlyphGeometry* glyph     = m_msdfData->FontGeo.getGlyph( c1 );
            m_msdfData->GlyphGeoLookuptableASCII[ c1 ] = glyph;

            if ( glyph == nullptr )
                continue;

            for ( unsigned char c2 = 0; c2 < 255; ++c2 ) {
                double advance = 0.0;
                if ( m_msdfData->FontGeo.getAdvance( advance, c1, c2 ) )
                    m_msdfData->KerningLookupTableASCII[ c1 ][ c2 ] = advance;
            }
        }
    }
}    // namespace InnoEngine
