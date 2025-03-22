#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Viewport.h"

namespace InnoEngine
{
    Viewport::Viewport( float left, float top, float width, float height ) :
        LeftOffset( left ), TopOffset( top ), Width( width ), Height( height ), MinDepth( 0.0f ), MaxDepth( 1.0f )
    {
    }

    Viewport::Viewport( float left, float top, float width, float height, float min_depth, float max_depth ) :
        LeftOffset( left ), TopOffset( top ), Width( width ), Height( height ), MinDepth( min_depth ), MaxDepth( max_depth )
    {
    }
}    // namespace InnoEngine
