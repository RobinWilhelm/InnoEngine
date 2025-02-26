#include "iepch.h"
#include "Application.h"

#include "CoreAPI.h"

#include "Window.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "OrthographicCamera.h"
#include "Scene.h"
#include "Shader.h"

#include "TetrisGameScene.h"

Application::~Application()
{
    m_scene.reset();
    m_assetManager.reset();
    m_renderer.reset();
    m_window.reset();
    m_camera.reset();
}

bool Application::create()
{
    if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
        IE_LOG_CRITICAL( "Failed to initialize SDL" );
        return false;
    }

    // create the core systems
    try {
        auto windowOpt = Window::create( { "Nasty Tetris", 720, 1040, 0 } );
        m_window       = std::move( windowOpt.value() );

        auto renderOpt = GPURenderer::create( m_window.get(), false );
        m_renderer     = std::move( renderOpt.value() );
        m_renderer->enable_vsync( true );

        auto assetManagerOpt = AssetManager::create( std::filesystem::current_path().append( "..\\..\\..\\assets\\" ), false );
        m_assetManager       = std::move( assetManagerOpt.value() );
        m_assetManager->add_repository<Sprite>( "Images" );

        auto shaderFormat = m_renderer->get_needed_shaderformat();
        m_assetManager->add_repository<Shader>( "Shaders" / shaderFormat.SubDirectory );

        m_camera = OrthographicCamera::create( 0, 720, 1040, 0 );

        publish_coreapi();
    } catch ( std::exception e ) {
        IE_LOG_CRITICAL( "Initialization failure %s", e.what() );
        return false;
    }

    m_scene = std::make_shared<TetrisGameScene>();
    set_simulation_target_frequency( 60 );

    return true;
}

bool Application::generate_frame()
{
    if ( m_mustQuit.load( std::memory_order_relaxed ) )
        return false;

    double newTime = static_cast<double>( SDL_GetTicksNS() ) / 1'000'000'000.0;
    m_frameContext.AccumulatedTime += newTime - m_frameContext.CurrentTime;
    m_frameContext.CurrentTime = newTime;
    while ( m_frameContext.AccumulatedTime >= m_frameContext.DeltaTime ) {
        if ( !fixed_update( m_frameContext ) ) {
            IE_LOG_CRITICAL( "Update failed!" );
            return false;
        }
        m_frameContext.AccumulatedTime -= m_frameContext.DeltaTime;
    }

    m_frameContext.InterpolationFactor = static_cast<float>( m_frameContext.AccumulatedTime / m_frameContext.DeltaTime );
    interpolate_and_collect_rendercommands( m_frameContext, m_camera.get() );

    return true;
}

void Application::interpolate_and_collect_rendercommands( const FrameContext& ctx, OrthographicCamera* pCamera [[maybe_unused]] )
{
    m_renderer->set_viewprojectionmatrix( pCamera->get_viewprojectionmatrix() );
    m_scene->interpolate_and_create_rendercommands( ctx.InterpolationFactor, m_renderer.get() );

    if ( m_renderer->is_multithreaded() ) {
        {
            std::unique_lock<std::mutex> ulock = m_renderer->wait_for_rendering_finished_and_hold_lock();
            m_renderer->submit_pipelines();
        }
        m_renderer->notify_renderthread();
    } else {
        m_renderer->submit_pipelines();
        m_renderer->process_pipelines();
    }
}

bool Application::handle_event( SDL_Event* event )
{
    if ( event->type == SDL_EVENT_QUIT )
        return false;
    else
        return m_scene->handle_event( event );
}

void Application::shutdown() {
    m_mustQuit.store(true, std::memory_order_relaxed);
}

void Application::set_simulation_target_frequency( uint16_t updatesPerSecond )
{
    m_frameContext.DeltaTime = 1.0f / updatesPerSecond;
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
    IE_LOG_CRITICAL( "%s", msg.c_str() );
    m_mustQuit.store( true, std::memory_order_relaxed );
}

bool Application::fixed_update( const FrameContext& ctx )
{
    m_scene->fixed_update( ctx.DeltaTime );
    return true;
}

void Application::publish_coreapi()
{
    CoreAPI& coreapi       = CoreAPI::get_instance();
    coreapi.m_app          = this;
    coreapi.m_assetManager = m_assetManager.get();
    coreapi.m_renderer     = m_renderer.get();
    coreapi.m_camera       = m_camera.get();
}
