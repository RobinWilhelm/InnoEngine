#include "Application.h"

namespace IE = InnoEngine;

class Sandbox : public IE::Application
{
public:
    // Inherited via Application
    IE::Result on_init() override;
    void       on_shutdown() override;
};
