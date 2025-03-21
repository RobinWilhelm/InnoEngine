#include "InnoEngine/Application.h"
#include "InnoEngine/AssetManager.h"

#include "DemoScene.h"

namespace IE = InnoEngine;

class Sandbox : public IE::Application
{
public:
    // Inherited via Application
    void       on_init_assets( IE::AssetManager* assetmanager ) override;
    IE::Result on_init() override;
    void       on_shutdown() override;

private:
    IE::Own<Demoscene> m_demoScene;
};
