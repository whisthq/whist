#include <SDL2/SDL.h>
#include <fractal/utils/logging.h>
#include "video.h"
#include "audio.h"

SDL_Thread* render_thread;
int run_render_thread = false;

int32_t multithreaded_render(void* opaque) {
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    if (init_video_renderer() != 0) {
        LOG_FATAL("Failed to initialize video renderer!");
    }

    while (run_render_thread) {
        // Render Audio
        // is thread-safe regardless of what other function calls are being made to audio
        render_audio();
        // Render video
        render_video();

        SDL_Delay(1);
    }

    return 0;
}

void init_renderer() {
    run_render_thread = true;
    render_thread =
        SDL_CreateThread(multithreaded_render, "multithreaded_render", NULL);
}

void destroy_renderer() {
    run_render_thread = false;
    SDL_WaitThread(render_thread, NULL);
}
