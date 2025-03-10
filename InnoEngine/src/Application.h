#pragma once
#include "SDL3/SDL_events.h"

#include "BaseTypes.h"
#include "Window.h"
#include "Profiler.h"
#include "RenderCommandBuffer.h"

#include <memory>
#include <string>
#include <condition_variable>
#include <filesystem>
#include <array>

namespace InnoEngine
{
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
        bool                   Headless = false;
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

        float get_fps();
        float get_timing( ProfilePoint element );

    private:
        virtual void   on_init_assets( AssetManager* assetmanager ) = 0;
        virtual Result on_init()                                    = 0;
        virtual void   on_shutdown()                                = 0;

        // Only relevant if application is running async
        // All data sharing between the threads has to be in here
        virtual void on_synchronize();

        // internal use only
        void synchronize();
        void poll_events();
        void handle_event( const SDL_Event& event );
        void update_layers();
        void render_layers();

        void run_async();

        void publish_coreapi();
        void update_profiledata();

    private:
        FrameTimingInfo         m_frameTimingInfo = {};
        Own<Window>             m_window;
        Own<GPURenderer>        m_renderer;
        Own<AssetManager>       m_assetManager;
        Own<OrthographicCamera> m_camera;
        Own<Profiler>           m_profiler;

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
        Own<Layer>          m_debugLayer;
        bool                m_debugui_active = false;

        std::array<float, static_cast<int>( ProfilePoint::Count )> m_profileData;
    };

}    // namespace InnoEngine
