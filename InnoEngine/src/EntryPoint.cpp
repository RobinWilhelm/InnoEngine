#include "iepch.h"

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"

#include "Application.h"

// these are needed somewhere so that they can be destructed by the unique_ptrs
#include "Renderer.h"
#include "Window.h"
#include "AssetManager.h"
#include "OrthographicCamera.h"

struct AppState
{
    Application gameMain;
};

SDL_AppResult SDL_AppInit( void** appstate, int argc [[maybe_unused]], char** argv [[maybe_unused]] )
{
    *appstate          = new AppState;
    AppState* appState = ( (AppState*)*appstate );
    if ( appState->gameMain.create() )
        return SDL_AppResult::SDL_APP_CONTINUE;

    return SDL_AppResult::SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppIterate( void* appstate )
{
    AppState* appState = ( (AppState*)appstate );
    if ( appState->gameMain.generate_frame() )
        return SDL_AppResult::SDL_APP_CONTINUE;

    return SDL_AppResult::SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppEvent( void* appstate, SDL_Event* event )
{
    AppState* appState = ( (AppState*)appstate );
    if ( appState->gameMain.handle_event( event ) )
        return SDL_AppResult::SDL_APP_CONTINUE;

    return SDL_AppResult::SDL_APP_FAILURE;
}

void SDL_AppQuit( void* appstate, SDL_AppResult result [[maybe_unused]] )
{
    AppState* appState = ( (AppState*)appstate );
    appState->gameMain.shutdown();
    delete appState;
}
