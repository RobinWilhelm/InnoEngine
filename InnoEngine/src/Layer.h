#pragma once
#include "SDL3/SDL_events.h"

#include <memory>

namespace InnoEngine
{
    class GPURenderer;

    class Layer
    {
    public:
        Layer()          = default;
        virtual ~Layer() = default;

        // not needed for now
        Layer( const Layer& other )            = delete;
        Layer( Layer&& other )                 = delete;
        Layer& operator=( const Layer& other ) = delete;
        Layer& operator=( Layer&& other )      = delete;

        virtual void update( double deltaTime )                           = 0;
        virtual void render( float interpFactor, GPURenderer* pRenderer ) = 0;
        virtual bool handle_event( const SDL_Event& pEvent )              = 0;    // return true when event should not be handled by deeper layers
    };

}    // namespace InnoEngine
