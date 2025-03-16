#pragma once
#include <SDL3/SDL_timer.h>

#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include "InnoEngine/Log.h"

#include <cstdint>
#include <memory>
#include <source_location>

namespace InnoEngine
{
    template <typename T>
    using Own = std::unique_ptr<T>;

    template <typename T>
    using Ref = std::shared_ptr<T>;

    enum class Result : int32_t
    {
        InvalidParameters   = -2,
        InitializationError = -1,
        Fail                = 0,
        Success             = 1,
        AlreadyInitialized  = 2,
    };

    constexpr const char* result_to_string( Result res )
    {
        switch ( res ) {
        case InnoEngine::Result::InvalidParameters:
            return "Invalid Parameters";
        case InnoEngine::Result::InitializationError:
            return "Initialization Error";
        case InnoEngine::Result::Fail:
            return "Fail";
        case InnoEngine::Result::Success:
            return "Success";
        case InnoEngine::Result::AlreadyInitialized:
            return "Already Initialized";
        default:
            return "Unknown result!";
        }
    }

    // clang-format off
#define IE_FAILED( x )  ( static_cast<int32_t>( x ) <= 0 )
#define IE_SUCCESS( x ) ( static_cast<int32_t>( x ) > 0 )
#define LOG_IF_FAILED( x ) { Result ie_result = ( x ); if ( IE_FAILED( ie_result ) ) { IE_LOG_DEBUG( "Result: %s", result_to_string( ie_result ) ); } }
#define RETURN_RESULT_IF_FAILED( x )  { Result ie_result = ( x ); if ( IE_FAILED( ie_result ) ) { return ie_result; } }
#define RETURN_IF_FAILED( x ) { if ( IE_FAILED( x ) ) { return; } }
    // clang-format on

    inline uint64_t get_tick_count()
    {
        return SDL_GetTicksNS();
    };

    constexpr int TicksPerSecond = 1'000'000'000;

    using FrameBufferIndex = int32_t;

    template <typename T>
    class RenderCommandQueue
    {
    public:
        RenderCommandQueue( size_t expectedQueueSize = 1000 )
        {
            m_collectQueue.reserve( expectedQueueSize );
            m_dispatchQueue.reserve( expectedQueueSize );
        }

        virtual ~RenderCommandQueue() = default;

        [[nodiscard]]
        T& create_entry()
        {
            grow_if_needed();
            return m_collectQueue.emplace_back();
        }

        void grow_if_needed()
        {
            if ( m_collectQueue.size() == m_collectQueue.capacity() ) {
                // Grow by a factor of 2.
                m_collectQueue.reserve( m_collectQueue.capacity() * 2 );
            }
        }

        void on_submit()
        {
            size_t items = m_collectQueue.size();
            if ( items != 0 ) {
                switch_buffers();

                // clear to get ready for collecting next frames commands
                m_collectQueue.clear();
                m_collectQueue.reserve( items );
            }
        }

        void switch_buffers()
        {
            // make a pointer swap
            m_collectQueue.swap( m_dispatchQueue );
        }

        std::vector<T>& get_collecting_queue()
        {
            return m_collectQueue;
        }

        std::vector<T>& get_dispatching_queue()
        {
            return m_dispatchQueue;
        }

    private:
        // force it to align to cache lines to prevent false sharing
        alignas( std::hardware_destructive_interference_size ) std::vector<T> m_collectQueue;
        alignas( std::hardware_destructive_interference_size ) std::vector<T> m_dispatchQueue;
    };

    template <typename T>
    class DoubleBuffered
    {
    public:
        void swap()
        {
            m_switch = !m_switch;
        }

        T& get_first()
        {
            return m_switch ? m_second : m_first;
        }

        T& get_second()
        {
            return m_switch ? m_first : m_second;
        }

    private:
        bool m_switch = false;
        // force it to align to cache lines to prevent false sharing
#pragma warning( push )
#pragma warning( disable :4324 )
        alignas( std::hardware_destructive_interference_size ) T m_first;
        alignas( std::hardware_destructive_interference_size ) T m_second;
#pragma warning( pop )
    };

}    // namespace InnoEngine
