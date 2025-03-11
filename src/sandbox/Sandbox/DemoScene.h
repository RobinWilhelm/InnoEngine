#include "InnoEngine/Layer.h"
#include "InnoEngine/Renderer.h"

namespace IE = InnoEngine;

class DemoLayer : public IE::Layer
{
    // Inherited via Layer
    void update( double deltaTime ) override;
    void render( float interpFactor, IE::GPURenderer* pRenderer ) override;
    bool handle_event( const SDL_Event& pEvent ) override;
};
