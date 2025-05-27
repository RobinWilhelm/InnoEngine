#include "InnoEngine/Application.h"
#include "InnoEngine/AssetManager.h"

#include "MainScene.h"
#include "GUI.h"

namespace IE = InnoEngine;

class SampleProject : public IE::Application
{
public:
    // Inherited via Application
    void       on_init_assets( IE::AssetManager* assetmanager ) override;
    IE::Result on_init() override;
    void       on_shutdown() override;

private:
    IE::Ref<World>     m_GameWorld;
    IE::Own<GameScene> m_GameLayer;
    IE::Own<GUI>       m_GUILayer;
};
