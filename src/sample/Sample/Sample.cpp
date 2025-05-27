#include "Sample.h"

#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/Shader.h"

#include "InnoEngine/AssetManager.h"

#include "World.h"
#include "GUI.h"

void SampleProject::on_init_assets( IE::AssetManager* assetmanager )
{
    assetmanager->add_repository<IE::Texture2D>( "images" );
    assetmanager->add_repository<IE::Shader>( "shaders" );
    assetmanager->add_repository<IE::Font>( "fonts" );
}

IE::Result SampleProject::on_init()
{
    m_GameWorld = World::create( { 3000.0f, 6000.0f } );
    m_GameLayer = GameScene::create( this, m_GameWorld );
    m_GUILayer  = GUI::create( this, m_GameWorld );
    push_layer( m_GameLayer.get() );
    push_layer( m_GUILayer.get() );
    return IE::Result::Success;
}

void SampleProject::on_shutdown()
{
}
