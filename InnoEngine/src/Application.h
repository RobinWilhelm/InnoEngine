#pragma once
#include "SDL3/SDL_events.h"

#include "Window.h"
#include "BaseTypes.h"
#include "RenderCommandBuffer.h"

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
    class Layer;

    struct FrameTimingInfo
    {
        int      FixedSimulationFrequency = 0;
        uint64_t CurrentTicks             = 0;      // time since app initialization
        uint64_t AccumulatedTicks         = 0;      // accumulated time since last update
        uint64_t DeltaTicks               = 0;      // time sinc last update in ticks
        double   DeltaTime                = 0.0;    // time since last update in seconds
        float    InterpolationFactor      = 0.0f;
    };

    struct CreationParams
    {
        Window::CreationParams WindowParams;
        int                    SimulationFrequency = 60;
        bool                   EnableVSync         = true;
        bool                   RunAsync            = false;    // create a separate thread for layer processing
        std::filesystem::path  AssetDirectory;
        bool                   AsyncAssetLoading = false;
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

        void enable_debugui( bool enable );

        void raise_critical_error( std::string msg );

    private:
        virtual Result on_init()  = 0;
        virtual void   shutdown() = 0;

        void poll_events();
        void handle_event( const SDL_Event& event );
        void update_layers();
        void render_layers();

        void run_async();

        void publish_coreapi();

    protected:
        FrameTimingInfo           m_frameTimingInfo = {};
        Owned<Window>             m_window;
        Owned<GPURenderer>        m_renderer;
        Owned<AssetManager>       m_assetManager;
        Owned<OrthographicCamera> m_camera;

        std::condition_variable m_updateCV;

        bool             m_initializationSucceded = false;
        std::atomic_bool m_mustQuit               = false;

        bool                    m_multiThreaded = false;
        std::thread             m_asyncApplicationThread;
        std::mutex              m_asyncMutex;
        std::condition_variable m_asyncThreadWaiting;
        std::condition_variable m_mainThreadWaiting;
        bool                    m_asyncThreadFinished = true;
        bool                    m_syncComplete        = false;

        // buffered events for async handling
        DoubleBuffered<std::vector<SDL_Event>> m_eventBuffer;

        std::vector<Layer*> m_layerStack;
        // keep debuglayer seperate and always topmost
        Owned<Layer>        m_debugLayer;
        bool                m_debugui_active = false;
    };

}    // namespace InnoEngine
