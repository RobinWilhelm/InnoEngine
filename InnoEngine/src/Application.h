#pragma once
#include "SDL3/SDL_events.h"

#include "Window.h"
#include "BaseTypes.h"

#include <memory>
#include <string>
#include <condition_variable>
#include <filesystem>

namespace InnoEngine
{
    class Window;
    class GPURenderer;
    class AssetManager;
    class OrthographicCamera;
    class Scene;

    struct FrameTimingInfo
    {
        int    FixedSimulationFrequency = 0;
        double CurrentTime              = 0.0;    // time since app initialization
        double AccumulatedTime          = 0.0;    // accumulated time since last update
        double DeltaTime                = 0.0;    // time since last update in seconds
        float  InterpolationFactor      = 0.0f;
    };

    struct CreationParams
    {
        Window::CreationParams WindowParams;
        int                    SimulationFrequency = 60;
        bool                   EnableVSync         = true;
        std::filesystem::path  AssetDirectory;
    };

    class Application
    {
    public:
        Application();
        virtual ~Application();

        Result init( const CreationParams& appParams );
        Result run();

        void set_simulation_target_frequency( int updatesPerSecond );

        Window*       get_window() const;
        GPURenderer*  get_renderer() const;
        AssetManager* get_assetmanager() const;

        void raise_critical_error( std::string msg );

    private:
        virtual Result on_init()                                                 = 0;
        virtual Result update( double deltaTime )                                = 0;
        virtual void   render( float interpFactor, OrthographicCamera* pCamera ) = 0;
        virtual bool   handle_event( SDL_Event* event )                          = 0;    // return true when the application should exit
        virtual void   shutdown()                                                = 0;

        void publish_coreapi();

    protected:
        FrameTimingInfo                     m_frameTimingInfo = {};
        std::unique_ptr<Window>             m_window;
        std::unique_ptr<GPURenderer>        m_renderer;
        std::unique_ptr<AssetManager>       m_assetManager;
        std::unique_ptr<OrthographicCamera> m_camera;

        std::condition_variable m_updateCV;

        std::shared_ptr<Scene> m_scene;

        bool             m_initializationSucceded = false;
        std::atomic_bool m_mustQuit               = false;
    };

}    // namespace InnoEngine
