#pragma once
#include "SDL3/SDL_events.h"

#include <memory>

namespace InnoEngine
{
    class GPURenderer;
    class Application;

    class Layer
    {
    public:
        Layer( Application* parent ) :
            m_Parent( parent ) { };
        virtual ~Layer() = default;

        // not needed for now
        Layer( const Layer& other )            = delete;
        Layer( Layer&& other )                 = delete;
        Layer& operator=( const Layer& other ) = delete;
        Layer& operator=( Layer&& other )      = delete;

        virtual void update( double delta_time )                          = 0;
        virtual void render( float interp_factor, GPURenderer* renderer ) = 0;
        virtual bool handle_event( const SDL_Event& event )               = 0;    // return true when event should not be handled by deeper layers

    protected:
        Application* m_Parent = nullptr;
    };

}    // namespace InnoEngine
