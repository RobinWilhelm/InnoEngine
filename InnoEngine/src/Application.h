#pragma once
#include "SDL3/SDL_events.h"

#include <memory>
#include <string>
#include <condition_variable>

class Window;
class GPURenderer;
class AssetManager;
class OrthographicCamera;
class Scene;

class Application
{
    struct FrameContext
    {
        double CurrentTime         = 0.0;    // time since app initialization
        double AccumulatedTime     = 0.0;    // accumulated time since last update
        double DeltaTime           = 0.0;    // time since last update in seconds
        float  InterpolationFactor = 0.0f;
    };

    enum class GameState
    {
        Menu = 0,
        Playing
    };

public:
    virtual ~Application();

    bool create();
    bool generate_frame();
    void interpolate_and_collect_rendercommands( const FrameContext& ctx, OrthographicCamera* pCamera );
    bool handle_event( SDL_Event* event );
    void shutdown();

    void set_simulation_target_frequency( uint16_t updatesPerSecond );

    Window*       get_window() const;
    GPURenderer*  get_renderer() const;
    AssetManager* get_assetmanager() const;

    void raise_critical_error( std::string msg );

private:
    bool fixed_update( const FrameContext& ctx );
    void publish_coreapi();

private:
    FrameContext                        m_frameContext = {};
    std::unique_ptr<Window>             m_window;
    std::unique_ptr<GPURenderer>        m_renderer;
    std::unique_ptr<AssetManager>       m_assetManager;
    std::unique_ptr<OrthographicCamera> m_camera;

    std::condition_variable m_updateCV;
    bool                    m_renderingFinished = true;

    GameState m_currentGameState = GameState::Menu;

    std::shared_ptr<Scene> m_scene;

    std::atomic_bool m_mustQuit = false;
};
