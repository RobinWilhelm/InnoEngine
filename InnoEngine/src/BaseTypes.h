#pragma once
#include <SDL3/SDL_timer.h>

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <cstdint>
#include <memory>

namespace InnoEngine
{
    template <typename T>
    using Own = std::unique_ptr<T>;

    template <typename T>
    using Ref = std::shared_ptr<T>;

    enum class Result : int32_t
    {
        InvalidParameters    = -3,
        InitializationError = -2,
        Fail                = -1,
        Unknown             = 0,
        Success             = 1,
        AlreadyInitialized  = 2,
    };

#define IE_FAILED( x )  ( static_cast<int32_t>(x) < 0 )
#define IE_SUCCESS( x ) ( static_cast<int32_t>(x) > 0 )

    inline uint64_t get_tick_count()
    {
        return SDL_GetTicksNS();
    };

    constexpr int TicksPerSecond = 1'000'000'000;

}    // namespace InnoEngine
