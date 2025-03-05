#pragma once
#include <SDL3/SDL_timer.h>

#include <cstdint>
#include <memory>

namespace InnoEngine
{
    template <typename T>
    using Owned = std::unique_ptr<T>;

    enum Result : int32_t
    {
        InitializationError = -2,
        Fail                = -1,
        Success             = 0,
    };

#define IE_FAILED( x )  ( x < 0 )
#define IE_SUCCESS( x ) ( x >= 0 )

    inline uint64_t getTickCount64(){ return SDL_GetTicksNS(); };
    constexpr int TicksPerSecond = 1'000'000'000;

#define ENABLE_PROFILER

}    // namespace InnoEngine
