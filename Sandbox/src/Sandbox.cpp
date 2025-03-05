#include "Sandbox.h"

#include "AssetManager.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Shader.h"

IE::Result Sandbox::on_init()
{
    m_assetManager->add_repository<IE::Sprite>("Images");

    auto shaderFormat = m_renderer->get_needed_shaderformat();
    m_assetManager->add_repository<IE::Shader>("Shaders" / shaderFormat.SubDirectory);
    return IE::Result::Success;
}

void Sandbox::on_shutdown()
{
}
