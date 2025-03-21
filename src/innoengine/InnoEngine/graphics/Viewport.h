#pragma once
#include "InnoEngine/BaseTypes.h"

#include "SDL3/SDL_gpu.h"

namespace InnoEngine
{
    struct Viewport
    {
        float LeftOffset = 0.0f;
        float TopOffset  = 0.0f;
        float Width      = 0.0f;
        float Height     = 0.0f;
        float MinDepth   = 0.0f;
        float MaxDepth   = 0.0f;

        Viewport() = default;
        Viewport( float left, float top, float width, float height );
        Viewport( float left, float top, float width, float height, float min_depth, float max_depth );
    };
}    // namespace InnoEngine
