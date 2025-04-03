#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Window.h"

namespace InnoEngine
{
    auto Window::create( const CreationParams& creationParams ) -> std::optional<std::unique_ptr<Window>>
    {
        std::unique_ptr<Window> window( new Window() );
        window->m_sdlWindow = SDL_CreateWindow( creationParams.title.c_str(), creationParams.width, creationParams.height, creationParams.flags );
        if ( window->m_sdlWindow == nullptr )
            return std::nullopt;

        window->m_params = creationParams;

        if ( SDL_GetWindowSize( window->m_sdlWindow, &window->m_ClientWidth, &window->m_ClientHeight ) == false ) {
            IE_LOG_CRITICAL( "Window creation failed: {}", SDL_GetError() );
            return std::nullopt;
        }

        IE_LOG_DEBUG( "Created Window: {} x {}", creationParams.width, creationParams.height );
        return window;
    }

    int Window::get_width() const
    {
        return static_cast<int>( m_params.width );
    }

    int Window::get_height() const
    {
        return static_cast<int>( m_params.height );
    }

    int Window::get_client_width() const
    {
        return m_ClientWidth;
    }

    int Window::get_client_height() const
    {
        return m_ClientHeight;
    }

    auto Window::get_sdlwindow() const -> SDL_Window*
    {
        return m_sdlWindow;
    }
}    // namespace InnoEngine
