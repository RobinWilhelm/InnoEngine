#include "InnoEngine/iepch.h"
#include "InnoEngine/InputSystem.h"

namespace InnoEngine
{
    auto InputSystem::create() -> std::optional<Own<InputSystem>>
    {
        return Own<InputSystem>( new InputSystem() );
    }

    void InputSystem::synchronize()
    {
        m_ConsumerInputData = m_ProducerInputData;

        // clicked state is only active for one frame -> reset it
        for ( auto& keystate : m_ProducerInputData.KeyStates ) {
            keystate.Clicked       = false;
            keystate.DoubleClicked = false;
        }

        m_ProducerInputData.MouseWheel = {};
        m_ProducerInputData.MouseMovement = {};
    }

    bool InputSystem::on_event( const SDL_Event& event )
    {
        switch ( event.type ) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            const SDL_Scancode scancode                    = event.key.scancode;
            m_ProducerInputData.KeyStates[ scancode ].Down = event.key.down;

            if ( event.key.type == SDL_EVENT_KEY_UP ) {
                TimeStampNS now = get_tick_count();

                m_ProducerInputData.KeyStates[ scancode ].Clicked = true;
                if ( now - m_ProducerInputData.KeyStates[ scancode ].LastClickTime < DoubleClickTimeNS )
                    m_ProducerInputData.KeyStates[ scancode ].DoubleClicked = true;

                m_ProducerInputData.KeyStates[ scancode ].LastClickTime = now;
            }
            return true;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            m_ProducerInputData.MousePos      = { event.motion.x, event.motion.y };
            m_ProducerInputData.MouseMovement = { event.motion.xrel, event.motion.yrel };
            return true;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            m_ProducerInputData.MouseButtonStates[ event.button.button ].Down   = event.button.down;
            m_ProducerInputData.MouseButtonStates[ event.button.button ].Clicks = event.button.clicks;
            return true;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            m_ProducerInputData.MouseWheel.x = event.wheel.x;
            m_ProducerInputData.MouseWheel.y = event.wheel.y;
            return true;
        }
        }

        return false;
    }

    bool InputSystem::is_key_down( SDL_Scancode scancode ) const
    {
        return get_key_state( scancode ).Down;
    }

    KeyState InputSystem::get_key_state( SDL_Scancode scancode ) const
    {
        return m_ConsumerInputData.KeyStates[ scancode ];
    }

    bool InputSystem::is_mouse_button_down( uint8_t mouse_button ) const
    {
        return get_mouse_button_state( mouse_button ).Down;
    }

    MouseButtonState InputSystem::get_mouse_button_state( uint8_t mouse_button ) const
    {
        return m_ConsumerInputData.MouseButtonStates[ mouse_button ];
    }

    const DXSM::Vector2 InputSystem::get_mouse_wheel_scroll() const
    {
        return m_ConsumerInputData.MouseWheel;
    }
}    // namespace InnoEngine
