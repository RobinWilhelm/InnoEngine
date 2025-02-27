#include "iepch.h"
#include "Application.h"

#include "CoreAPI.h"

#include "Window.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "OrthographicCamera.h"
#include "Scene.h"
#include "Shader.h"

namespace InnoEngine
{
    Application::Application()
    {
    }

    Application::~Application()
    {
        m_scene.reset();
        m_assetManager.reset();
        m_renderer.reset();
        m_window.reset();
        m_camera.reset();
    }

    void Application::render( float interpFactor, OrthographicCamera* pCamera [[maybe_unused]] )
    {
        m_renderer->set_viewprojectionmatrix( pCamera->get_viewprojectionmatrix() );
        m_scene->render( interpFactor, m_renderer.get() );

        if ( m_renderer->is_multithreaded() ) {
            {
                std::unique_lock<std::mutex> ulock = m_renderer->wait_for_rendering_finished_and_hold_lock();
                m_renderer->submit_pipelines();
            }
            m_renderer->notify_renderthread();
        }
        else {
            m_renderer->submit_pipelines();
            m_renderer->process_pipelines();
        }
    }

    void Application::shutdown()
    {
        m_mustQuit.store( true, std::memory_order_relaxed );
    }

    Result Application::init( const CreationParams& appParams )
    {
        if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
            IE_LOG_CRITICAL( "Failed to initialize SDL" );
            return Result::Fail;
        }

        Result result = Result::Success;
        // create the core systems
        try {
            if ( appParams.WindowParams.height != 0 && appParams.WindowParams.width != 0 ) {
                auto windowOpt = Window::create( appParams.WindowParams );
                m_window       = std::move( windowOpt.value() );
                m_camera       = OrthographicCamera::create( 0, appParams.WindowParams.width, appParams.WindowParams.height, 0 );
            }

            auto renderOpt = GPURenderer::create( m_window.get(), false );
            m_renderer     = std::move( renderOpt.value() );
            m_renderer->enable_vsync( appParams.EnableVSync );

            auto assetManagerOpt = AssetManager::create( appParams.AssetDirectory, false );
            m_assetManager       = std::move( assetManagerOpt.value() );

            set_simulation_target_frequency( appParams.SimulationFrequency );
            result = on_init();
            publish_coreapi();
        } catch ( std::exception e ) {
            IE_LOG_CRITICAL("Initialization failure {}", e.what());
            return Result::Fail;
        }

        return result;
    }

    Result Application::run()
    {
        while ( m_mustQuit.load( std::memory_order_relaxed ) == false ) {

            SDL_Event event;
            while ( SDL_PollEvent( &event ) != 0 ) {
                if ( event.type == SDL_EventType::SDL_EVENT_QUIT ) {
                    m_mustQuit = true;
                }
                if ( handle_event( &event ) == Result::Fail ) {
                    m_mustQuit = true;
                }
            }

            double newTime = static_cast<double>( SDL_GetTicksNS() ) / 1'000'000'000.0;
            m_frameTimingInfo.AccumulatedTime += newTime - m_frameTimingInfo.CurrentTime;

            if ( m_frameTimingInfo.FixedSimulationFrequency == 0 ) {
                m_frameTimingInfo.DeltaTime   = newTime - m_frameTimingInfo.CurrentTime;
                m_frameTimingInfo.CurrentTime = newTime;

                if ( !update( m_frameTimingInfo.DeltaTime ) ) {
                    IE_LOG_CRITICAL( "Fixed update failed!" );
                    return Result::Fail;
                }

                m_frameTimingInfo.InterpolationFactor = 0.0f;
            }
            else {
                m_frameTimingInfo.CurrentTime = newTime;
                while ( m_frameTimingInfo.AccumulatedTime >= m_frameTimingInfo.DeltaTime ) {
                    if ( !update( m_frameTimingInfo.DeltaTime ) ) {
                        IE_LOG_CRITICAL( "Fixed update failed!" );
                        return Result::Fail;
                    }
                    m_frameTimingInfo.AccumulatedTime -= m_frameTimingInfo.DeltaTime;
                }
                m_frameTimingInfo.InterpolationFactor = static_cast<float>( m_frameTimingInfo.AccumulatedTime / m_frameTimingInfo.DeltaTime );
            }

            render( m_frameTimingInfo.InterpolationFactor, m_camera.get() );
        }

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

    void Application::raise_critical_error( std::string msg )
    {
        IE_LOG_CRITICAL( "Critical error: {}", msg.c_str() );
        m_mustQuit.store( true, std::memory_order_relaxed );
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
