#pragma once
#include "SDL3/SDL_events.h"

#include <memory>

namespace InnoEngine
{
    class GPURenderer;

    class Scene
    {
    public:
        Scene()          = default;
        virtual ~Scene() = default;

        // not needed for now
        Scene( const Scene& other )            = delete;
        Scene( Scene&& other )                 = delete;
        Scene& operator=( const Scene& other ) = delete;
        Scene& operator=( Scene&& other )      = delete;

        virtual void update( double deltaTime )                           = 0;
        virtual void render( float interpFactor, GPURenderer* pRenderer ) = 0;
        virtual bool handle_event( SDL_Event* pEvent )                    = 0;
    };

}    // namespace InnoEngine
