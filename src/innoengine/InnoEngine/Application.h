#pragma once
#include "SDL3/SDL_events.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/utility/Profiler.h"
#include "InnoEngine/graphics/Viewport.h"

#include <memory>
#include <string>
#include <condition_variable>
#include <filesystem>
#include <array>

namespace InnoEngine
{
    class GPURenderer;
    class AssetManager;
    class Camera;
    class Layer;
    class InputSystem;
    class CameraController;

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
        bool                   Headless            = false;
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

        void                   set_simulation_target_frequency( int updatesPerSecond );
        const FrameTimingInfo& get_frame_timings() const;

        Window*       get_window() const;
        GPURenderer*  get_renderer() const;
        AssetManager* get_assetmanager() const;

        void enable_debugui( bool enable );
        bool running_mutithreaded() const;

        void raise_critical_error( std::string msg );

        float get_fps();
        float get_timing( ProfilePoint element );
        void  push_layer( Layer* layer );

        // not threadsafe, only to be called from layers
        void register_camera( Ref<Camera> camera );
        void unregister_camera( Ref<Camera> camera );
        void register_cameracontroller( Ref<CameraController> camera_controller );
        void unregister_cameracontroller( Ref<CameraController> camera_controller );

        const Viewport& get_fullscreen_viewport() const;

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
        void create_update();
        void update( double delta_time );
        void render_layers();

        void run_async();

        void publish_coreapi();
        void update_profiledata();

    private:
        FrameTimingInfo   m_FrameTimingInfo = {};
        Own<Window>       m_Window;
        Own<GPURenderer>  m_Renderer;
        Own<AssetManager> m_AssetManager;
        Own<Profiler>     m_Profiler;
        Own<InputSystem>  m_InputSystem;

        std::vector<Ref<Camera>>           m_Cameras;
        std::vector<Ref<CameraController>> m_CameraControllers;
        Viewport                           m_FullscreenDefaultViewport;

        bool             m_InitializationSucceded = false;
        std::atomic_bool m_MustQuit               = false;

        bool                    m_MultiThreaded = false;
        std::thread             m_AsyncApplicationThread;
        std::mutex              m_AsyncMutex;
        std::condition_variable m_AsyncThreadWaiting;
        std::condition_variable m_MainThreadWaiting;
        bool                    m_AsyncThreadFinished = false;
        bool                    m_SyncComplete        = true;

        // buffered events for async handling
        DoubleBuffered<std::vector<SDL_Event>> m_EventBuffer;

        std::vector<Layer*> m_LayerStack;
        // keep debuglayer seperate and always topmost
        Own<Layer>          m_DebugLayer;
        bool                m_DebugUIEnabled = false;

        std::array<float, static_cast<int>( ProfilePoint::Count )> m_ProfileData = {};
    };

}    // namespace InnoEngine
