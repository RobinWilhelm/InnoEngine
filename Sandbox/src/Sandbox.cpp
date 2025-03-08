#include "Sandbox.h"

#include "AssetManager.h"
#include "Renderer.h"
#include "Texture2D.h"
#include "Shader.h"


void Sandbox::on_init_assets(IE::AssetManager* assetmanager)
{
    assetmanager->add_repository<IE::Texture2D>("Images");
    assetmanager->add_repository<IE::Shader>("Shaders");
}

IE::Result Sandbox::on_init()
{
    return IE::Result::Success;
}

void Sandbox::on_shutdown()
{
}
