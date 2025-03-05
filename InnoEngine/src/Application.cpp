#include "iepch.h"
#include "Application.h"

#include "CoreAPI.h"

#include "Window.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "OrthographicCamera.h"
#include "Layer.h"
#include "Shader.h"
#include "DebugUI.h"
#include "RenderCommandBuffer.h"
#include "Profiler.h"

namespace InnoEngine
{
#ifdef ENABLE_PROFILER
    #define PROFILE_START( x ) ( m_profiler->start( x ) )
    #define PROFILE_STOP( x )  ( m_profiler->stop( x ) )
#else
    #define PROFILE_START( x )
    #define PROFILE_STOP( x )
#endif

    Application::Application()
    {
    }

    Application::~Application()
    {
        m_debugLayer.reset();
        m_assetManager.reset();
        m_renderer.reset();
        m_window.reset();
        m_camera.reset();

        SDL_Quit();
    }

    void Application::render_layers()
    {
        for ( auto layer : m_layerStack )
            layer->render( m_frameTimingInfo.InterpolationFactor, m_renderer.get() );

        // always last and top most
        if ( m_debugui_active )
            m_debugLayer->render( m_frameTimingInfo.DeltaTime, m_renderer.get() );
    }

    void Application::run_async()
    {
        while ( m_mustQuit.load( std::memory_order_relaxed ) == false ) {

            {    // hold mutex while the async update is doing work
                std::unique_lock<std::mutex> ulock( m_asyncMutex );
                m_asyncThreadWaiting.wait( ulock, [ this ]() { return m_syncComplete; } );

                m_profiler->start( ProfileElements::UpdateThreadTotal );

                for ( const auto& event : m_eventBuffer.get_second() )
                    handle_event( event );

                m_profiler->start( ProfileElements::Update );
                update_layers();
                m_camera->update();
                m_profiler->stop( ProfileElements::Update );

                render_layers();

                m_profiler->stop( ProfileElements::UpdateThreadTotal );

                m_syncComplete        = false;
                m_asyncThreadFinished = true;
            }
            m_mainThreadWaiting.notify_one();
        }
    }

    void Application::on_shutdown()
    {
        m_mustQuit.store( true, std::memory_order_relaxed );
    }

    Result Application::init( const CreationParams& appParams )
    {
        IE_LOG_INFO( "Starting application at: \"{}\"", std::filesystem::current_path().string() );
        if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
            IE_LOG_CRITICAL( "Failed to initialize SDL" );
            return Result::Fail;
        }

        Result result = Result::Fail;
        // create the core systems
        try {
            if ( appParams.WindowParams.height != 0 && appParams.WindowParams.width != 0 ) {
                auto windowOpt = Window::create( appParams.WindowParams );
                m_window       = std::move( windowOpt.value() );
                m_camera       = OrthographicCamera::create( 0, appParams.WindowParams.width, appParams.WindowParams.height, 0 );
            }

            auto renderOpt = GPURenderer::create( m_window.get() );
            m_renderer     = std::move( renderOpt.value() );
            m_renderer->enable_vsync( appParams.EnableVSync );

            auto assetManagerOpt = AssetManager::create( appParams.AssetDirectory, appParams.AsyncAssetLoading );
            m_assetManager       = std::move( assetManagerOpt.value() );

#ifdef ENABLE_PROFILER
            auto profilerOpt = Profiler::create();
            m_profiler       = std::move( profilerOpt.value() );
#endif

            set_simulation_target_frequency( appParams.SimulationFrequency );
            m_multiThreaded = appParams.RunAsync;

            result = on_init();
            publish_coreapi();
        } catch ( std::exception e ) {
            IE_LOG_CRITICAL( "Application initialization failed: \"{}\"", e.what() );
            return Result::InitializationError;
        }

        IE_LOG_INFO( "Application initialization complete" );
        m_initializationSucceded = true;
        return result;
    }

    Result Application::run()
    {
        if ( m_initializationSucceded == false )
            return Result::InitializationError;

        if ( m_multiThreaded ) {
            m_asyncApplicationThread = std::thread( &Application::run_async, this );
            IE_LOG_DEBUG( "Created async application thread" );
        }

        while ( m_mustQuit.load( std::memory_order_relaxed ) == false ) {

            m_profiler->start( ProfileElements::TotalFrame );
            m_profiler->start( ProfileElements::MainThreadTotal );

            poll_events();

            if ( m_multiThreaded == false ) {
                update_profiledata();

                m_profiler->start( ProfileElements::Update );
                update_layers();
                m_camera->update();
                m_profiler->stop( ProfileElements::Update );

                m_profiler->start( ProfileElements::Render );
                render_layers();

                m_renderer->set_camera_matrix( m_camera->get_viewprojectionmatrix() );
                m_renderer->submit_pipelines();
                m_renderer->process_pipelines();

                m_profiler->stop( ProfileElements::Render );
            }
            else {
                {    // hold mutex while we are synchronising data with the async thread
                    std::unique_lock<std::mutex> ulock( m_asyncMutex );
                    m_mainThreadWaiting.wait( ulock, [ this ]() { return m_asyncThreadFinished; } );

                    synchronize();

                    m_syncComplete        = true;
                    m_asyncThreadFinished = false;
                }
                m_asyncThreadWaiting.notify_one();

                m_profiler->start( ProfileElements::Render );
                m_renderer->process_pipelines();
                m_profiler->stop( ProfileElements::Render );
            }

            m_profiler->stop( ProfileElements::MainThreadTotal );
            m_profiler->stop( ProfileElements::TotalFrame );
            m_profiler->update();
        }

        if ( m_multiThreaded )
            m_asyncApplicationThread.join();

        IE_LOG_INFO( "Shutting down" );
        on_shutdown();
        return Result::Success;
    }

    void Application::set_simulation_target_frequency( int updateFrequency )
    {
        m_frameTimingInfo.FixedSimulationFrequency = updateFrequency;
        m_frameTimingInfo.DeltaTime                = 1.0f / updateFrequency;
        m_frameTimingInfo.DeltaTicks               = updateFrequency > 0 ? TicksPerSecond / updateFrequency : 0;
    }

    Window* Application::get_window() const
    {
        return m_window.get();
    }

    GPURenderer* Application::get_renderer() const
    {
        return m_renderer.get();
    }

    AssetManager* Application::get_assetmanager() const
    {
        return m_assetManager.get();
    }

    void Application::enable_debugui( bool enabled )
    {
        if ( m_debugLayer == nullptr ) {

            auto opt     = DebugUI::create( m_renderer.get() );
            m_debugLayer = std::move( opt.value() );
        }

        IE_ASSERT( m_debugLayer != nullptr );
        IE_LOG_DEBUG( "Debug UI {}", enabled ? "enabled" : "disabled" );
        m_debugui_active = enabled;
    }

    void Application::raise_critical_error( std::string msg )
    {
        IE_LOG_CRITICAL( "Critical error: {}", msg.c_str() );
        m_mustQuit.store( true, std::memory_order_relaxed );
    }

    float Application::get_fps()
    {
        return 1.0f / m_profileData[ static_cast<uint32_t>( ProfileElements::TotalFrame ) ];
    }

    float Application::get_timing( ProfileElements element )
    {
        return m_profileData[ static_cast<uint32_t>( element ) ];
    }

    void Application::on_synchronize()
    {
    }

    void Application::synchronize()
    {
        m_renderer->set_camera_matrix( m_camera->get_viewprojectionmatrix() );
        m_eventBuffer.swap();
        m_eventBuffer.get_first().clear();
        m_renderer->submit_pipelines();

        update_profiledata();

        on_synchronize();

        m_syncComplete        = true;
        m_asyncThreadFinished = false;
    }

    void Application::poll_events()
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) != 0 ) {

            if ( m_multiThreaded ) {
                m_eventBuffer.get_first().emplace_back( event );
            }
            else {
                handle_event( event );
            }

            // always handle quit events
            if ( event.type == SDL_EVENT_QUIT ||
                 ( m_window != nullptr &&
                   event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                   event.window.windowID == SDL_GetWindowID( m_window->get_sdlwindow() ) ) ) {
                IE_LOG_DEBUG( "Shutdown requested" );
                m_mustQuit = true;
                break;
            }
        }
    }

    void Application::handle_event( const SDL_Event& event )
    {
        // pass events through in reverse order
        bool handled = false;
        if ( m_debugui_active )
            handled = m_debugLayer->handle_event( event );

        auto revIt = m_layerStack.rbegin();
        while ( revIt != m_layerStack.rend() && handled == false )
            handled = ( *revIt )->handle_event( event );
    }

    void Application::update_layers()
    {
        // double newTime = static_cast<double>( SDL_GetTicksNS() ) / 1'000'000'000.0;
        uint64_t newTime = getTickCount64();
        m_frameTimingInfo.AccumulatedTicks += newTime - m_frameTimingInfo.CurrentTicks;

        if ( m_frameTimingInfo.FixedSimulationFrequency == 0 ) {
            m_frameTimingInfo.DeltaTime    = static_cast<double>( newTime - m_frameTimingInfo.CurrentTicks ) / TicksPerSecond;
            m_frameTimingInfo.CurrentTicks = newTime;

            for ( auto layer : m_layerStack ) {
                layer->update( m_frameTimingInfo.DeltaTime );
            }

            // always last and top most
            if ( m_debugui_active )
                m_debugLayer->update( m_frameTimingInfo.DeltaTime );

            m_frameTimingInfo.InterpolationFactor = 0.0f;
        }
        else {
            m_frameTimingInfo.CurrentTicks = newTime;
            while ( m_frameTimingInfo.AccumulatedTicks >= m_frameTimingInfo.DeltaTicks ) {
                for ( auto layer : m_layerStack ) {
                    layer->update( m_frameTimingInfo.DeltaTime );
                }

                // always last and top most
                if ( m_debugui_active )
                    m_debugLayer->update( m_frameTimingInfo.DeltaTime );

                m_frameTimingInfo.AccumulatedTicks -= m_frameTimingInfo.DeltaTicks;
            }
            m_frameTimingInfo.InterpolationFactor = static_cast<float>( m_frameTimingInfo.AccumulatedTicks ) / m_frameTimingInfo.DeltaTicks;
        }
    }

    void Application::publish_coreapi()
    {
        CoreAPI& coreapi       = CoreAPI::get_instance();
        coreapi.m_app          = this;
        coreapi.m_assetManager = m_assetManager.get();
        coreapi.m_renderer     = m_renderer.get();
        coreapi.m_camera       = m_camera.get();
    }

    void Application::update_profiledata()
    {
        IE_ASSERT( m_profiler != nullptr );
        for ( size_t i = 0; i < static_cast<uint32_t>( ProfileElements::COUNT ); ++i ) {
            m_profileData[ i ] = m_profiler->get_average( static_cast<ProfileElements>( i ) ) / TicksPerSecond;
        }
    }
}    // namespace InnoEngine
