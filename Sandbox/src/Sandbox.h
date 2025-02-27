#include "Application.h"

namespace IE = InnoEngine;

class Sandbox : public IE::Application
{

public:
    // Inherited via Application
    IE::Result on_init() override;
    IE::Result update(double deltaTime) override;
    void render(float interpFactor, IE::OrthographicCamera* pCamera) override;
    IE::Result handle_event(SDL_Event* event) override;
    void shutdown() override;
};