#pragma once
#include "InnoEngine/BaseTypes.h"
#include "SDL3/SDL_events.h"

#include <array>

namespace InnoEngine
{
    struct KeyState
    {
        TimeStampNS LastClickTime;
        bool        Down;
        bool        Clicked;
        bool        DoubleClicked;
    };

    struct MouseButtonState
    {
        bool    Down;
        uint8_t Clicks;
    };

    constexpr uint64_t DoubleClickTimeNS = 500000;    // 0.5 seconds

    class InputSystem
    {
        InputSystem() = default;

    public:
        static auto create() -> std::optional<Own<InputSystem>>;

        void synchronize();
        bool on_event( const SDL_Event& event );

        bool     is_key_down( SDL_Scancode scancode ) const;
        KeyState get_key_state( SDL_Scancode scancode ) const;

        bool             is_mouse_button_down( uint8_t mouse_button ) const;
        MouseButtonState get_mouse_button_state( uint8_t mouse_button ) const;
        DXSM::Vector2    get_mouse_position() const;
        DXSM::Vector2    get_mouse_movement() const;

        const DXSM::Vector2 get_mouse_wheel_scroll() const;

    private:
        struct InputData
        {
            std::array<KeyState, SDL_SCANCODE_COUNT> KeyStates;

            DXSM::Vector2                     MousePos;
            DXSM::Vector2                     MouseMovement;
            DXSM::Vector2                     MouseWheel;
            std::array<MouseButtonState, 256> MouseButtonStates;
        };

        // force it to align to cache lines to prevent false sharing
#pragma warning( push )
#pragma warning( disable :4324 )
        alignas( std::hardware_destructive_interference_size ) InputData m_ProducerInputData;
        alignas( std::hardware_destructive_interference_size ) InputData m_ConsumerInputData;
#pragma warning( pop )
    };
}    // namespace InnoEngine
