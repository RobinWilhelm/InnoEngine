#include "Sample.h"

#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/Shader.h"

#include "InnoEngine/AssetManager.h"

void SampleProject::on_init_assets( IE::AssetManager* assetmanager )
{
    assetmanager->add_repository<IE::Texture2D>( "images" );
    assetmanager->add_repository<IE::Shader>( "shaders" );
    assetmanager->add_repository<IE::Font>( "fonts" );
}

IE::Result SampleProject::on_init()
{
    m_demoScene = std::make_unique<GameScene>(this);
    push_layer( m_demoScene.get() );
    return IE::Result::Success;
}

void SampleProject::on_shutdown()
{
}
