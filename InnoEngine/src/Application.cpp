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

namespace InnoEngine
{
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

                for ( const auto& event : m_eventBuffer.get_second() )
                    handle_event( event );

                update_layers();
                m_camera->update();
                render_layers();

                m_syncComplete        = false;
                m_asyncThreadFinished = true;
            }
            m_mainThreadWaiting.notify_one();
        }
    }

    void Application::shutdown()
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

        if ( m_multiThreaded )          {
            m_asyncApplicationThread = std::thread(&Application::run_async, this );
            IE_LOG_DEBUG("Created async application thread");
        }

        while ( m_mustQuit.load( std::memory_order_relaxed ) == false ) {

            poll_events();

            if ( m_multiThreaded == false ) {
                update_layers();
                m_camera->update();
                render_layers();

                m_renderer->set_camera_matrix( m_camera->get_viewprojectionmatrix() );
                m_renderer->submit_pipelines();
                m_renderer->process_pipelines();
            }
            else {
                DXSM::Matrix mainCameraMatrix;

                // sync new events with update thread and recieve rendercommands
                {
                    std::unique_lock<std::mutex> ulock( m_asyncMutex );
                    m_mainThreadWaiting.wait( ulock, [ this ]() { return m_asyncThreadFinished; } );

                    mainCameraMatrix = m_camera->get_viewprojectionmatrix();
                    m_eventBuffer.swap();
                    m_renderer->submit_pipelines();

                    m_syncComplete        = true;
                    m_asyncThreadFinished = false;
                }
                m_asyncThreadWaiting.notify_one();

                m_renderer->set_camera_matrix( mainCameraMatrix );
                m_renderer->process_pipelines();
            }
        }

        if ( m_multiThreaded )
            m_asyncApplicationThread.join();

        IE_LOG_INFO( "Shutting down" );
        shutdown();
        return Result::Success;
    }

    void Application::set_simulation_target_frequency( int updatesPerSecond )
    {
        m_frameTimingInfo.FixedSimulationFrequency = updatesPerSecond;
        m_frameTimingInfo.DeltaTime                = 1.0f / updatesPerSecond;
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
        m_debugui_active = enabled;
    }

    void Application::raise_critical_error( std::string msg )
    {
        IE_LOG_CRITICAL( "Critical error: {}", msg.c_str() );
        m_mustQuit.store( true, std::memory_order_relaxed );
    }

    void Application::poll_events()
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) != 0 ) {

            if ( m_multiThreaded ) {
                m_eventBuffer.get_first().clear();
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
        double newTime = static_cast<double>( SDL_GetTicksNS() ) / 1'000'000'000.0;
        m_frameTimingInfo.AccumulatedTime += newTime - m_frameTimingInfo.CurrentTime;

        if ( m_frameTimingInfo.FixedSimulationFrequency == 0 ) {
            m_frameTimingInfo.DeltaTime   = newTime - m_frameTimingInfo.CurrentTime;
            m_frameTimingInfo.CurrentTime = newTime;

            for ( auto layer : m_layerStack ) {
                layer->update( m_frameTimingInfo.DeltaTime );
            }

            // always last and top most
            if ( m_debugui_active )
                m_debugLayer->update( m_frameTimingInfo.DeltaTime );

            m_frameTimingInfo.InterpolationFactor = 0.0f;
        }
        else {
            m_frameTimingInfo.CurrentTime = newTime;

            while ( m_frameTimingInfo.AccumulatedTime >= m_frameTimingInfo.DeltaTime ) {
                for ( auto layer : m_layerStack ) {
                    layer->update( m_frameTimingInfo.DeltaTime );
                }

                // always last and top most
                if ( m_debugui_active )
                    m_debugLayer->update( m_frameTimingInfo.DeltaTime );

                m_frameTimingInfo.AccumulatedTime -= m_frameTimingInfo.DeltaTime;
            }
            m_frameTimingInfo.InterpolationFactor = static_cast<float>( m_frameTimingInfo.AccumulatedTime / m_frameTimingInfo.DeltaTime );
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
}    // namespace InnoEngine
