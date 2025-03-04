#pragma once
#include <cstdint>
#include <memory>

namespace InnoEngine
{
    template <typename T>
    using Owned = std::unique_ptr<T>;

    enum Result : int32_t
    {
        InitializationError = -2,
        Fail    = -1,
        Success = 0,
    };

#define IE_FAILED( x )  ( x < 0 )
#define IE_SUCCESS( x ) ( x >= 0 )
}    // namespace InnoEngine
