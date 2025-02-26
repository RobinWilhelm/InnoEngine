#pragma once

#include "SDL3/SDL_log.h"

// TODO refactor to own warning system
#define IE_LOG_DEBUG( ... )    SDL_LogDebug( SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__ )
#define IE_LOG_INFO( ... )     SDL_LogInfo( SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__ )
#define IE_LOG_WARNING( ... )  SDL_LogWarn( SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__ )
#define IE_LOG_ERROR( ... )    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__ )
#define IE_LOG_CRITICAL( ... ) SDL_LogCritical( SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__ ) 
