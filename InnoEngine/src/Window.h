#pragma once
#include "SDL3/SDL_video.h"

#include <string>
#include <memory>
#include <optional>

struct WindowCreationParameters
{
    std::string     title = "InnoEngine";
    uint16_t        width = 1280, height = 720;
    SDL_WindowFlags flags = 0;
};

class Window
{
    Window() = default;

public:
    [[nodiscard]]
    static std::optional<std::unique_ptr<Window>> create( const WindowCreationParameters& creationParams );

    uint32_t    get_width() const;
    uint32_t    get_height() const;
    SDL_Window* get_sdlwindow() const;

private:
    std::string     m_title;
    uint16_t        m_width = 0, m_height = 0;
    SDL_WindowFlags m_flags     = 0;
    SDL_Window*     m_sdlWindow = nullptr;
};
