#pragma once
#include <vector>
#include <unordered_map>

#undef min
#undef max

#pragma warning( disable :4458; disable :4505 )
#include "msdf-atlas-gen/msdf-atlas-gen.h"
using namespace msdf_atlas;
#pragma warning( default :4458; default :4505 )

namespace InnoEngine
{
    struct MSDFData
    {
        // Storage for glyph geometry and their coordinates in the atlas
        std::vector<msdf_atlas::GlyphGeometry> GlyphGeo;

        // FontGeometry is a helper class that loads a set of glyphs from a single font.
        // It can also be used to get additional font metrics, kerning information, etc.
        msdf_atlas::FontGeometry FontGeo;


        // fast lookup tables because the msdf lib is using ordinary maps for those
        // speeds it up by about 50%
        std::unordered_map<msdf_atlas::unicode_t, const msdf_atlas::GlyphGeometry*> GlyphGeoLookupTable;
        std::unordered_map<uint64_t, double>                                        KerningLookupTable;

        const msdf_atlas::GlyphGeometry* get_glyph( msdf_atlas::unicode_t codepoint )
        {
            auto geo_it = GlyphGeoLookupTable.find( codepoint );
            if ( geo_it != GlyphGeoLookupTable.end() ) {
                return geo_it->second;
            }
            const msdf_atlas::GlyphGeometry* glyph_geo = FontGeo.getGlyph( codepoint );
            GlyphGeoLookupTable[ codepoint ]           = glyph_geo;
            return glyph_geo;
        }

        bool get_advance( double& advance, msdf_atlas::unicode_t character, msdf_atlas::unicode_t next_character )
        {
            union Key
            {
                uint64_t value;

                struct Pair
                {
                    int index_1;
                    int index_2;
                } pair;
            };

            const GlyphGeometry* glyph1;
            const GlyphGeometry* glyph2;

            if ( !( ( glyph1 = get_glyph( character ) ) && ( glyph2 = get_glyph( next_character ) ) ) )
                return false;
            advance = glyph1->getAdvance();

            Key key;
            key.pair.index_1 = glyph1->getIndex();
            key.pair.index_2 = glyph2->getIndex();

            auto it = KerningLookupTable.find( key.value );
            if ( it != KerningLookupTable.end() ) {
                advance += it->second;
            }
            else {
                if ( FontGeo.getAdvance( advance, character, next_character ) == false ) {
                    return false;
                }
                else {
                    KerningLookupTable[ key.value ] = advance - glyph1->getAdvance();
                }
            }
            return true;
        }
    };
}    // namespace InnoEngine
