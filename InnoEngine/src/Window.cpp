#include "iepch.h"
#include "Window.h"

std::optional<std::unique_ptr<Window>> Window::create( const WindowCreationParameters& creationParams )
{
    std::unique_ptr<Window> window( new Window() );
    window->m_title  = creationParams.title;
    window->m_width  = creationParams.width;
    window->m_height = creationParams.height;
    window->m_flags  = creationParams.flags;

    window->m_sdlWindow = SDL_CreateWindow( window->m_title.c_str(), window->m_width, window->m_height, window->m_flags );
    if ( window->m_sdlWindow == nullptr )
        return std::nullopt;

    IE_LOG_INFO("Created Window: %ux%u", window->m_width, window->m_height);
    return window;
}

uint32_t Window::get_width() const
{
    return m_width;
}

uint32_t Window::get_height() const
{
    return m_height;
}

SDL_Window* Window::get_sdlwindow() const
{
    return m_sdlWindow;
}
