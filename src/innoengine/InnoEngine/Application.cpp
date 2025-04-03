#include "InnoEngine/iepch.h"
#include "InnoEngine/Application.h"

#include "InnoEngine/CoreAPI.h"

#include "InnoEngine/AssetManager.h"
#include "InnoEngine/DebugUI.h"
#include "InnoEngine/Layer.h"
#include "InnoEngine/graphics/OrthographicCamera.h"
#include "InnoEngine/utility/Profiler.h"
#include "InnoEngine/graphics/RenderCommandBuffer.h"
#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Shader.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/InputSystem.h"
#include "InnoEngine/graphics/DefaultCameraController.h"
#include "InnoEngine/graphics/RenderContext.h"

namespace InnoEngine
{
    Application::Application() { }

    Application::~Application()
    {
        if ( m_Renderer )
            m_Renderer->wait_for_gpu_idle();
        m_DebugLayer.reset();
        m_AssetManager.reset();
        m_Renderer.reset();
        m_Window.reset();

        SDL_Quit();
    }

    void Application::render_layers()
    {
        ProfileScoped render_layers( ProfilePoint::LayerRender );
        m_Renderer->begin_collection();
        for ( auto layer : m_LayerStack )
            layer->render( m_FrameTimingInfo.InterpolationFactor, m_Renderer.get() );

        // always last and top most
        if ( m_DebugUIEnabled )
            m_DebugLayer->render( m_FrameTimingInfo.InterpolationFactor, m_Renderer.get() );

        m_Renderer->end_collection();
    }

    void Application::run_async()
    {
        while ( m_MustQuit.load( std::memory_order_relaxed ) == false ) {

            {    // hold mutex while the async update is doing work
                std::unique_lock<std::mutex> ulock( m_AsyncMutex );
                m_AsyncThreadWaiting.wait( ulock, [ this ]() { return m_SyncComplete; } );

                ProfileScoped update_thread( ProfilePoint::UpdateThreadTotal );

                for ( const auto& event : m_EventBuffer.get_consumer_data() )
                    handle_event( event );

                create_update();
                render_layers();

                m_SyncComplete        = false;
                m_AsyncThreadFinished = true;
            }
            m_MainThreadWaiting.notify_one();
        }
    }

    void Application::on_shutdown()
    {
        m_MustQuit.store( true, std::memory_order_relaxed );
    }

    Result Application::init( const CreationParams& appParams )
    {
        if ( appParams.Headless != false || appParams.WindowParams.height == 0 || appParams.WindowParams.width == 0 ) {
            IE_LOG_CRITICAL( "Headless mode not fully supported yet" );
            return Result::InvalidParameters;
        }

        IE_LOG_INFO( "Starting application at: \"{}\"", std::filesystem::current_path().string() );
        if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
            IE_LOG_CRITICAL( "Failed to initialize SDL" );
            return Result::InitializationError;
        }

        Result result = Result::Fail;
        // create the core systems
        try {
            auto asset_manager_optional = AssetManager::create( appParams.AssetDirectory, appParams.AsyncAssetLoading );
            m_AssetManager              = std::move( asset_manager_optional.value() );

            auto window_optional = Window::create( appParams.WindowParams );
            m_Window             = std::move( window_optional.value() );

            auto render_optional = GPURenderer::create();
            m_Renderer           = std::move( render_optional.value() );

            auto profiler_optional = Profiler::create();
            m_Profiler             = std::move( profiler_optional.value() );

            auto input_system_opt = InputSystem::create();
            m_InputSystem         = std::move( input_system_opt.value() );

            m_FullscreenDefaultViewport = Viewport( 0, 0, static_cast<float>( m_Window->get_client_width() ), static_cast<float>( m_Window->get_client_height() ) );

            m_DefaultCamera = OrthographicCamera::create( { m_FullscreenDefaultViewport.Width, m_FullscreenDefaultViewport.Height } );
            register_camera( m_DefaultCamera );

            RenderContextSpecifications render_ctx_ui = {};
            render_ctx_ui.Camera                      = m_DefaultCamera;
            render_ctx_ui.Viewport                    = m_FullscreenDefaultViewport;
            m_RCHScreen                               = m_Renderer->create_rendercontext( render_ctx_ui );

            on_init_assets( m_AssetManager.get() );
            publish_coreapi();

            result = m_Renderer->initialize( m_Window.get(), m_AssetManager.get() );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Renderer initialization failed: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            result = m_Renderer->enable_vsync( appParams.EnableVSync );
            if ( IE_FAILED( result ) ) {
                IE_LOG_ERROR( "Failed to set vsync option: {}", static_cast<uint32_t>( result ) );
            }

            set_simulation_target_frequency( appParams.SimulationFrequency );
            m_MultiThreaded = appParams.RunAsync;

            result = on_init();

        } catch ( std::exception e ) {
            IE_LOG_CRITICAL( "Application initialization failed: \"{}\"", e.what() );
            return Result::InitializationError;
        }

        if ( IE_SUCCESS( result ) ) {
            IE_LOG_INFO( "Application initialization succeded" );
            m_InitializationSucceded = true;
        }
        else {
            IE_LOG_INFO( "Application initialization failed" );
        }
        return result;
    }

    Result Application::run()
    {
        if ( m_InitializationSucceded == false )
            return Result::InitializationError;

        if ( m_MultiThreaded ) {
            m_AsyncApplicationThread = std::thread( &Application::run_async, this );
            IE_LOG_DEBUG( "Created async application thread" );
        }

        while ( m_MustQuit.load( std::memory_order_relaxed ) == false ) {

            m_Profiler->start( ProfilePoint::MainThreadTotal );

            poll_events();

            if ( m_MultiThreaded == false ) {
                update_profiledata();
                m_InputSystem->synchronize();
                create_update();

                render_layers();
                m_Renderer->synchronize();
                m_Renderer->render();
            }
            else {
                {
                    ProfileScoped wait_and_sync( ProfilePoint::WaitAndSynchronize );

                    // hold mutex while we are synchronising data with the async thread
                    std::unique_lock<std::mutex> ulock( m_AsyncMutex );
                    m_MainThreadWaiting.wait( ulock, [ this ]() { return m_AsyncThreadFinished; } );
                    synchronize();
                }
                m_AsyncThreadWaiting.notify_one();
                m_Renderer->render();
            }

            m_Profiler->stop( ProfilePoint::MainThreadTotal );
            m_Profiler->update();
        }

        if ( m_MultiThreaded )
            m_AsyncApplicationThread.join();

        IE_LOG_INFO( "Shutting down" );
        on_shutdown();
        return Result::Success;
    }

    void Application::set_simulation_target_frequency( int updateFrequency )
    {
        m_FrameTimingInfo.FixedSimulationFrequency = updateFrequency;
        m_FrameTimingInfo.DeltaTime                = 1.0f / updateFrequency;
        m_FrameTimingInfo.DeltaTicks               = updateFrequency > 0 ? TicksPerSecond / updateFrequency : 0;
    }

    const FrameTimingInfo& Application::get_frame_timings() const
    {
        return m_FrameTimingInfo;
    }

    Window* Application::get_window() const
    {
        return m_Window.get();
    }

    GPURenderer* Application::get_renderer() const
    {
        return m_Renderer.get();
    }

    AssetManager* Application::get_assetmanager() const
    {
        return m_AssetManager.get();
    }

    void Application::enable_debugui( bool enabled )
    {
        if ( m_DebugLayer == nullptr ) {

            auto opt     = DebugUI::create( m_Renderer.get() );
            m_DebugLayer = std::move( opt.value() );
        }

        IE_ASSERT( m_DebugLayer != nullptr );
        IE_LOG_DEBUG( "Debug UI {}", enabled ? "enabled" : "disabled" );
        m_DebugUIEnabled = enabled;
    }

    bool Application::running_mutithreaded() const
    {
        return m_MultiThreaded;
    }

    void Application::raise_critical_error( std::string msg )
    {
        IE_LOG_CRITICAL( "Critical error: {}", msg.c_str() );
        m_MustQuit.store( true, std::memory_order_relaxed );
    }

    float Application::get_fps()
    {
        return 1.0f / m_ProfileData[ static_cast<uint32_t>( ProfilePoint::MainThreadTotal ) ];
    }

    float Application::get_timing( ProfilePoint element )
    {
        return m_ProfileData[ static_cast<uint32_t>( element ) ];
    }

    void Application::push_layer( Layer* layer )
    {
        m_LayerStack.push_back( layer );
    }

    void Application::register_camera( Ref<Camera> camera )
    {
        // check if it is already inserted
        auto it = m_Cameras.begin();
        while ( it != m_Cameras.end() ) {
            if ( ( *it ) == camera ) {
                IE_LOG_WARNING( "Camera already registered!" );
                return;
            }
            ++it;
        }
        m_Cameras.push_back( camera );
        IE_LOG_DEBUG( "Camera registered" );
    }

    void Application::unregister_camera( Ref<Camera> camera )
    {
        auto it = m_Cameras.begin();
        while ( it != m_Cameras.end() ) {
            if ( ( *it ) == camera ) {
                m_Cameras.erase( it );
                IE_LOG_DEBUG( "Camera unregistered" );
                return;
            }
            ++it;
        }
    }

    void Application::register_cameracontroller( Ref<CameraController> camera_controller )
    {
        // check if it is already inserted
        auto it = m_CameraControllers.begin();
        while ( it != m_CameraControllers.end() ) {
            if ( ( *it ) == camera_controller ) {
                IE_LOG_WARNING( "Camera controller already registered!" );
                return;
            }
            ++it;
        }
        m_CameraControllers.push_back( camera_controller );
        IE_LOG_DEBUG( "Camera controller registered" );
    }

    void Application::unregister_cameracontroller( Ref<CameraController> camera_controller )
    {
        auto it = m_CameraControllers.begin();
        while ( it != m_CameraControllers.end() ) {
            if ( ( *it ) == camera_controller ) {
                m_CameraControllers.erase( it );
                IE_LOG_DEBUG( "Camera controller unregistered" );
                return;
            }
            ++it;
        }
    }

    const Viewport& Application::get_fullscreen_viewport() const
    {
        return m_FullscreenDefaultViewport;
    }

    Ref<Camera> Application::get_default_camera() const
    {
        return m_DefaultCamera;
    }

    RenderContextHandle Application::get_fullscreen_rch() const
    {
        return m_RCHScreen;
    }

    void Application::on_synchronize() { }

    void Application::synchronize()
    {
        m_InputSystem->synchronize();
        m_EventBuffer.swap();
        m_EventBuffer.get_producer_data().clear();
        m_Renderer->synchronize();

        update_profiledata();

        on_synchronize();

        m_SyncComplete        = true;
        m_AsyncThreadFinished = false;
    }

    void Application::poll_events()
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) != 0 ) {

            m_InputSystem->on_event( event );
            if ( m_MultiThreaded ) {
                m_EventBuffer.get_producer_data().emplace_back( event );
            }
            else {
                handle_event( event );
            }

            // always handle quit events
            if ( event.type == SDL_EVENT_QUIT || ( event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                                                   m_Window != nullptr &&
                                                   event.window.windowID == SDL_GetWindowID( m_Window->get_sdlwindow() ) ) ) {
                IE_LOG_DEBUG( "Shutdown requested" );
                m_MustQuit = true;
                break;
            }
        }
    }

    void Application::handle_event( const SDL_Event& event )
    {
        ProfileScoped layer_event( ProfilePoint::LayerEvent );
        // pass events through in reverse order
        if ( m_DebugUIEnabled ) {
            if ( m_DebugLayer->handle_event( event ) )
                return;
        }

        for ( auto& cam_controller : m_CameraControllers ) {
            if ( cam_controller->handle_event( event ) )
                return;
        }

        auto revIt = m_LayerStack.rbegin();
        while ( revIt != m_LayerStack.rend() ) {
            if ( ( *revIt )->handle_event( event ) )
                return;
            ++revIt;
        }
    }

    void Application::create_update()
    {
        uint64_t current_time = get_tick_count();
        m_FrameTimingInfo.AccumulatedTicks += current_time - m_FrameTimingInfo.CurrentTicks;

        if ( m_FrameTimingInfo.FixedSimulationFrequency == 0 ) {
            m_FrameTimingInfo.DeltaTime    = static_cast<double>( current_time - m_FrameTimingInfo.CurrentTicks ) / TicksPerSecond;
            m_FrameTimingInfo.CurrentTicks = current_time;
            update( m_FrameTimingInfo.DeltaTime );
            m_FrameTimingInfo.InterpolationFactor = 0.0f;
        }
        else {
            m_FrameTimingInfo.CurrentTicks = current_time;
            while ( m_FrameTimingInfo.AccumulatedTicks >= m_FrameTimingInfo.DeltaTicks ) {
                update( m_FrameTimingInfo.DeltaTime );
                m_FrameTimingInfo.AccumulatedTicks -= m_FrameTimingInfo.DeltaTicks;
            }
            m_FrameTimingInfo.InterpolationFactor = static_cast<float>( m_FrameTimingInfo.AccumulatedTicks ) / m_FrameTimingInfo.DeltaTicks;
        }
    }

    void Application::update( double delta_time )
    {
        ProfileScoped layer_update( ProfilePoint::LayerUpdate );
        for ( auto& cam_controller : m_CameraControllers )
            cam_controller->update( delta_time );

        for ( auto layer : m_LayerStack ) {
            layer->update( delta_time );
        }

        // always last and top most
        if ( m_DebugUIEnabled )
            m_DebugLayer->update( delta_time );

        for ( auto& cam : m_Cameras )
            cam->update();
    }

    void Application::publish_coreapi()
    {
        CoreAPI& coreapi       = CoreAPI::get_instance();
        coreapi.m_App          = this;
        coreapi.m_AssetManager = m_AssetManager.get();
        coreapi.m_Renderer     = m_Renderer.get();
        coreapi.m_Profiler     = m_Profiler.get();
        coreapi.m_Input        = m_InputSystem.get();
    }

    void Application::update_profiledata()
    {
        IE_ASSERT( m_Profiler != nullptr );
        for ( size_t i = 0; i < static_cast<uint32_t>( ProfilePoint::Count ); ++i ) {
            m_ProfileData[ i ] = static_cast<float>( m_Profiler->get_average( static_cast<ProfilePoint>( i ) ) ) / TicksPerSecond;
        }
    }
}    // namespace InnoEngine
