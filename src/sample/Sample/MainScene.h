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

#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/math_functions.h"

namespace IE = InnoEngine;

class World;
class SampleProject;
class PlayerCameraController;

class GameScene : public IE::Layer
{
public:
    GameScene( SampleProject* parent );
    ~GameScene();

    // Inherited via Layer
    void update( double deltaTime ) override;
    void render( float interpFactor, IE::GPURenderer* pRenderer ) override;
    bool handle_event( const SDL_Event& pEvent ) override;

private:
    InnoEngine::Ref<PlayerCameraController> m_CameraController;
    InnoEngine::Ref<World>                  m_GameWorld;
};
