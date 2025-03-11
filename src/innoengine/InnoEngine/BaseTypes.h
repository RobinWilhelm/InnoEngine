#pragma once
#include <SDL3/SDL_timer.h>

#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include "Log.h"

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
    case InnoEngine::Result::InvalidParameters: return "Invalid Parameters";
    case InnoEngine::Result::InitializationError: return "Initialization Error";
    case InnoEngine::Result::Fail: return "Fail";
    case InnoEngine::Result::Success: return "Success";
    case InnoEngine::Result::AlreadyInitialized: return "Already Initialized";
    default: return "Unknown result!";
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

}    // namespace InnoEngine
