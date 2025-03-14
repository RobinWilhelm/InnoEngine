#include "Sandbox.h"

#include "InnoEngine/Renderer.h"
#include "InnoEngine/Texture2D.h"
#include "InnoEngine/Shader.h"

#include "InnoEngine/AssetManager.h"

void Sandbox::on_init_assets( IE::AssetManager* assetmanager )
{
    assetmanager->add_repository<IE::Texture2D>( "images" );
    assetmanager->add_repository<IE::Shader>( "shaders" );
    assetmanager->add_repository<IE::Font>( "fonts" );
}

IE::Result Sandbox::on_init()
{
    m_demoScene = std::make_unique<DemoLayer>();
    push_layer( m_demoScene.get() );
    return IE::Result::Success;
}

void Sandbox::on_shutdown()
{
}
