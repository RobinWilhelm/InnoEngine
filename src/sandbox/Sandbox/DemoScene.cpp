#include "DemoScene.h"
#include "InnoEngine/Renderer.h"

void DemoLayer::update(double delta_time)
{
    (void)delta_time;
}

void DemoLayer::render(float interp_factor, IE::GPURenderer* renderer)
{
    (void)interp_factor;
    (void)renderer;
    renderer->add_clear({0.0f, 0.5f,0.0f, 1.0f});
}

bool DemoLayer::handle_event(const SDL_Event& event)
{
    (void)event;
    return false;
}
