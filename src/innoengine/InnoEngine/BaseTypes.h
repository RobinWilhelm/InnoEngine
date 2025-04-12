#pragma once
#include "SDL3/SDL_timer.h"

#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include "InnoEngine/utility/Log.h"

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

    enum class TextureFormat
    {
        Invalid = 0,
        RGBA,
        RGBX
    };

    enum class Origin
    {
        Middle = 0,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        RotationOrigin,
    };

    // transform the given coordinates and origin to a origin in the middle for the shaders
    constexpr const DXSM::Vector2 origin_transform( Origin origin, const DXSM::Vector2& position, const DXSM::Vector2& size, const DXSM::Vector2& rotation_origin = { 0.5f, 0.5f } )
    {
        DXSM::Vector2 new_pos = position;
        switch ( origin ) {
        case Origin::Middle:
            new_pos.x -= size.x * 0.5f;
            new_pos.y -= size.y * 0.5f;
            break;
        case Origin::TopLeft:
            new_pos.y -= size.y;
            break;
        case Origin::TopRight:
            new_pos.x += size.x;
            new_pos.y -= size.y;
            break;
        case Origin::BottomLeft:
            return new_pos;
            break;
        case Origin::BottomRight:
            new_pos.x -= size.x;
            break;
        case Origin::RotationOrigin:
            new_pos -= rotation_origin;
            break;
        }
        return new_pos;
    }

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

    using TimeStampNS = uint64_t;

    inline TimeStampNS get_tick_count()
    {
        return SDL_GetTicksNS();
    };

    constexpr uint64_t TicksPerSecond = 1'000'000'000;

    template <typename T>
    class DoubleBuffered
    {
    public:
        void swap()
        {
            m_switch = !m_switch;
        }

        T& get_producer_data()
        {
            return m_switch ? m_second : m_first;
        }

        const T& get_consumer_data() const
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

    using RenderCommandBufferIndexType                                     = uint32_t;
    constexpr RenderCommandBufferIndexType InvalidRenderCommandBufferIndex = ( std::numeric_limits<uint32_t>::max )();

    struct RenderCommandBase
    {
        float                        Depth        = 0;
        RenderCommandBufferIndexType ContextIndex = InvalidRenderCommandBufferIndex;
    };

}    // namespace InnoEngine
