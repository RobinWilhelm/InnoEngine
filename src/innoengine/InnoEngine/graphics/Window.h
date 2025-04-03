#pragma once
#include "SDL3/SDL_video.h"

#include "InnoEngine/BaseTypes.h"

#include <string>
#include <memory>
#include <optional>

namespace InnoEngine
{
    class Window
    {
        Window() = default;

    public:
        struct CreationParams
        {
            std::string     title = "InnoEngine";
            uint16_t        width = 1280, height = 720;
            SDL_WindowFlags flags = 0;
        };

        [[nodiscard]]
        static auto create( const CreationParams& creationParams ) -> std::optional<Own<Window>>;

        int get_width() const;
        int get_height() const;

        int get_client_width() const;
        int get_client_height() const;

        SDL_Window* get_sdlwindow() const;

    private:
        CreationParams m_params;
        SDL_Window*    m_sdlWindow    = nullptr;
        int            m_ClientWidth  = 0;
        int            m_ClientHeight = 0;
    };

}    // namespace InnoEngine
