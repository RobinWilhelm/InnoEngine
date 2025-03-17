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

namespace InnoEngine
{
    Application::Application() { }

    Application::~Application()
    {
        if ( m_renderer )
            m_renderer->wait_for_gpu_idle();
        m_debugLayer.reset();
        m_assetManager.reset();
        m_camera.reset();
        m_renderer.reset();
        m_window.reset();

        SDL_Quit();
    }

    void Application::render_layers()
    {
        m_profiler->start( ProfilePoint::LayerRender );
        for ( auto layer : m_layerStack )
            layer->render( m_frameTimingInfo.InterpolationFactor, m_renderer.get() );

        // always last and top most
        if ( m_debugui_active )
            m_debugLayer->render( m_frameTimingInfo.InterpolationFactor, m_renderer.get() );
        m_profiler->stop( ProfilePoint::LayerRender );
    }

    void Application::run_async()
    {
        while ( m_mustQuit.load( std::memory_order_relaxed ) == false ) {

            {    // hold mutex while the async update is doing work
                std::unique_lock<std::mutex> ulock( m_asyncMutex );
                m_asyncThreadWaiting.wait( ulock, [ this ]() { return m_syncComplete; } );

                m_profiler->start( ProfilePoint::UpdateThreadTotal );

                for ( const auto& event : m_eventBuffer.get_second() )
                    handle_event( event );

                update_layers();
                m_camera->update();
                render_layers();

                m_syncComplete        = false;
                m_asyncThreadFinished = true;
            }
            m_mainThreadWaiting.notify_one();

            m_profiler->stop( ProfilePoint::UpdateThreadTotal );
        }
    }

    void Application::on_shutdown()
    {
        m_mustQuit.store( true, std::memory_order_relaxed );
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
            auto assetManagerOpt = AssetManager::create( appParams.AssetDirectory, appParams.AsyncAssetLoading );
            m_assetManager       = std::move( assetManagerOpt.value() );

            auto windowOpt = Window::create( appParams.WindowParams );
            m_window       = std::move( windowOpt.value() );
            m_camera       = OrthographicCamera::create( 0, appParams.WindowParams.width, appParams.WindowParams.height, 0 );

            auto renderOpt = GPURenderer::create();
            m_renderer     = std::move( renderOpt.value() );

            auto profilerOpt = Profiler::create();
            m_profiler       = std::move( profilerOpt.value() );

            on_init_assets( m_assetManager.get() );
            publish_coreapi();

            result = m_renderer->initialize( m_window.get(), m_assetManager.get() );
            if ( IE_FAILED( result ) ) {
                IE_LOG_CRITICAL( "Renderer initialization failed: {}", static_cast<uint32_t>( result ) );
                return result;
            }

            result = m_renderer->enable_vsync( appParams.EnableVSync );
            if ( IE_FAILED( result ) ) {
                IE_LOG_ERROR( "Failed to set vsync option: {}", static_cast<uint32_t>( result ) );
            }

            set_simulation_target_frequency( appParams.SimulationFrequency );
            m_multiThreaded = appParams.RunAsync;

            result = on_init();

        } catch ( std::exception e ) {
            IE_LOG_CRITICAL( "Application initialization failed: \"{}\"", e.what() );
            return Result::InitializationError;
        }

        if ( IE_SUCCESS( result ) ) {
            IE_LOG_INFO( "Application initialization succeded" );
            m_initializationSucceded = true;
        }
        else {
            IE_LOG_INFO( "Application initialization failed" );
        }
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

            m_profiler->start( ProfilePoint::MainThreadTotal );

            poll_events();

            if ( m_multiThreaded == false ) {
                update_profiledata();

                update_layers();
                m_camera->update();

                render_layers();

                m_profiler->start( ProfilePoint::ProcessRenderCommands );
                m_renderer->on_synchronize();
                m_renderer->render();
                m_profiler->stop( ProfilePoint::ProcessRenderCommands );
            }
            else {
                m_profiler->start( ProfilePoint::WaitAndSynchronize );
                {    // hold mutex while we are synchronising data with the async thread
                    std::unique_lock<std::mutex> ulock( m_asyncMutex );
                    m_mainThreadWaiting.wait( ulock, [ this ]() { return m_asyncThreadFinished; } );

                    synchronize();
                }
                m_asyncThreadWaiting.notify_one();
                m_profiler->stop( ProfilePoint::WaitAndSynchronize );

                m_profiler->start( ProfilePoint::ProcessRenderCommands );
                m_renderer->render();
                m_profiler->stop( ProfilePoint::ProcessRenderCommands );
            }

            m_profiler->stop( ProfilePoint::MainThreadTotal );
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

    const FrameTimingInfo& Application::get_frame_timings() const
    {
        return m_frameTimingInfo;
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

    bool Application::running_mutithreaded() const
    {
        return m_multiThreaded;
    }

    void Application::raise_critical_error( std::string msg )
    {
        IE_LOG_CRITICAL( "Critical error: {}", msg.c_str() );
        m_mustQuit.store( true, std::memory_order_relaxed );
    }

    float Application::get_fps()
    {
        return 1.0f / m_profileData[ static_cast<uint32_t>( ProfilePoint::MainThreadTotal ) ];
    }

    float Application::get_timing( ProfilePoint element )
    {
        return m_profileData[ static_cast<uint32_t>( element ) ];
    }

    void Application::push_layer( Layer* layer )
    {
        m_layerStack.push_back( layer );
    }

    void Application::on_synchronize() { }

    void Application::synchronize()
    {
        m_eventBuffer.swap();
        m_eventBuffer.get_first().clear();
        m_renderer->on_synchronize();

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
            if ( event.type == SDL_EVENT_QUIT || ( m_window != nullptr && event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID( m_window->get_sdlwindow() ) ) ) {
                IE_LOG_DEBUG( "Shutdown requested" );
                m_mustQuit = true;
                break;
            }
        }
    }

    void Application::handle_event( const SDL_Event& event )
    {
        m_profiler->start( ProfilePoint::LayerEvent );
        // pass events through in reverse order
        bool handled = false;
        if ( m_debugui_active )
            handled = m_debugLayer->handle_event( event );

        auto revIt = m_layerStack.rbegin();
        while ( revIt != m_layerStack.rend() && handled == false ) {
            handled = ( *revIt )->handle_event( event );
            ++revIt;
        }
        m_profiler->stop( ProfilePoint::LayerEvent );
    }

    void Application::update_layers()
    {
        // double newTime = static_cast<double>( SDL_GetTicksNS() ) / 1'000'000'000.0;
        uint64_t newTime = get_tick_count();
        m_frameTimingInfo.AccumulatedTicks += newTime - m_frameTimingInfo.CurrentTicks;

        if ( m_frameTimingInfo.FixedSimulationFrequency == 0 ) {
            m_frameTimingInfo.DeltaTime    = static_cast<double>( newTime - m_frameTimingInfo.CurrentTicks ) / TicksPerSecond;
            m_frameTimingInfo.CurrentTicks = newTime;

            m_profiler->start( ProfilePoint::LayerUpdate );
            for ( auto layer : m_layerStack ) {
                layer->update( m_frameTimingInfo.DeltaTime );
            }

            // always last and top most
            if ( m_debugui_active )
                m_debugLayer->update( m_frameTimingInfo.DeltaTime );

            m_frameTimingInfo.InterpolationFactor = 0.0f;
            m_profiler->stop( ProfilePoint::LayerUpdate );
        }
        else {
            m_frameTimingInfo.CurrentTicks = newTime;
            while ( m_frameTimingInfo.AccumulatedTicks >= m_frameTimingInfo.DeltaTicks ) {

                m_profiler->start( ProfilePoint::LayerUpdate );
                for ( auto layer : m_layerStack ) {
                    layer->update( m_frameTimingInfo.DeltaTime );
                }

                // always last and top most
                if ( m_debugui_active )
                    m_debugLayer->update( m_frameTimingInfo.DeltaTime );

                m_frameTimingInfo.AccumulatedTicks -= m_frameTimingInfo.DeltaTicks;
                m_profiler->stop( ProfilePoint::LayerUpdate );
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
        coreapi.m_profiler     = m_profiler.get();
    }

    void Application::update_profiledata()
    {
        IE_ASSERT( m_profiler != nullptr );
        for ( size_t i = 0; i < static_cast<uint32_t>( ProfilePoint::Count ); ++i ) {
            m_profileData[ i ] = static_cast<float>( m_profiler->get_average( static_cast<ProfilePoint>( i ) ) ) / TicksPerSecond;
        }
    }
}    // namespace InnoEngine
