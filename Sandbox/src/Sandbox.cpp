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

IE::Result Sandbox::update(double deltaTime)
{
    return IE::Result::Success;
}

void Sandbox::render(float interpFactor, IE::OrthographicCamera* pCamera)
{
}

bool Sandbox::handle_event(SDL_Event* event)
{
    return false;
}

void Sandbox::shutdown()
{
}
