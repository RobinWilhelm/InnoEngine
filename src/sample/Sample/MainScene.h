#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/Layer.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/Sprite.h"
#include "InnoEngine/graphics/Camera.h"

#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/RenderContext.h"

namespace IE = InnoEngine;

class SampleProject;
class PlayerCameraController;

class GameScene : public IE::Layer
{
public:
    GameScene( SampleProject* parent );

    // Inherited via Layer
    void update( double deltaTime ) override;
    void render( float interpFactor, IE::GPURenderer* pRenderer ) override;
    bool handle_event( const SDL_Event& pEvent ) override;

private:
    InnoEngine::Ref<PlayerCameraController> m_CameraController;

    const uint32_t m_SceneWidth  = 3000;
    const uint32_t m_SceneHeight = 6000;
};
