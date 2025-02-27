#pragma once

#include "SDL3/SDL_assert.h"

// #ifndef IE_RELEASE
#define IE_ENABLE_ASSERTS
// #endif

// TODO refactor to own assert system
#ifdef IE_ENABLE_ASSERTS
    #define IE_ASSERT( expression ) SDL_assert_release( expression )
#else
    #define IE_ASSERT( ... )
#endif
