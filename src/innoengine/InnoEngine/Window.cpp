#include "InnoEngine/iepch.h"
#include "InnoEngine/Window.h"

namespace InnoEngine
{
    auto Window::create( const CreationParams& creationParams ) -> std::optional<std::unique_ptr<Window>>
    {
        std::unique_ptr<Window> window( new Window() );
        window->m_sdlWindow = SDL_CreateWindow( creationParams.title.c_str(), creationParams.width, creationParams.height, creationParams.flags );
        if ( window->m_sdlWindow == nullptr )
            return std::nullopt;

        window->m_params = creationParams;
        IE_LOG_DEBUG( "Created Window: {} x {}", creationParams.width, creationParams.height );
        return window;
    }

    int Window::width() const
    {
        return static_cast<int>( m_params.width );
    }

    int Window::height() const
    {
        return static_cast<int>( m_params.height );
    }

    auto Window::get_sdlwindow() const -> SDL_Window*
    {
        return m_sdlWindow;
    }
}    // namespace InnoEngine
