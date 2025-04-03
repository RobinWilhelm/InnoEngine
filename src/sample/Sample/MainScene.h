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

class SampleProject;
class PlayerCameraController;

struct Asteroid
{
    float         Size;
    DXSM::Vector2 Position;
    DXSM::Vector2 PositionNext;
    DXSM::Vector2 Velocity;
    b2BodyId      PhysicsBodyId = {};
};

class AntiAsteroidArtillery
{
public:
    virtual void update( double delta_time ) = 0;

protected:
    DXSM::Vector2 m_Position;
    float         m_CannonElevation;
    float         m_ReloadProgress;
};

class DefaultCannon : public AntiAsteroidArtillery
{
public:
    virtual void update( double delta_time ) override;
};

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

    const uint32_t m_SceneWidth        = 3000;
    const uint32_t m_SceneHeight       = 6000;
    const float    m_AsteroidSpawnTime = 1.0f;

    float                 m_LastAsteroidSpawn = 0.0f;
    std::vector<Asteroid> m_Asteroids;

    b2WorldId m_PhysicsWorldId  = {};
    b2BodyId  m_PhysicsGroundId = {};
};
