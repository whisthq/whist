#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

typedef struct SDLFrontendContext {
} SDLFrontendContext;

static atomic_int sdl_initialized = ATOMIC_VAR_INIT(0);

static int sdl_frontend_init(WhistFrontend* frontend) {
    // We only need to initialize SDL once.
    if (atomic_fetch_or(&sdl_initialized, 1) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            LOG_FATAL("Could not initialize SDL - %s", SDL_GetError());
        }
        // If we have initialized SDL, then on exit we should clean up.
        atexit(SDL_Quit);
    }

    frontend->context = safe_malloc(sizeof(SDLFrontendContext));
    return 0;
}

static void sdl_frontend_destroy(WhistFrontend* frontend) {
    free(frontend->context);
}

static const WhistFrontendFunctionTable sdl_function_table = {
    .init = sdl_frontend_init,
    .destroy = sdl_frontend_destroy,
};

const WhistFrontendFunctionTable* sdl_get_function_table(void) {
    return &sdl_function_table;
}
