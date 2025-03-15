#pragma once
#include <vector>

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
    };
}