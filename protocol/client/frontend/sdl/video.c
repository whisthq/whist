#include "common.h"
#include "native.h"
#include <whist/utils/lodepng.h>

// Little-endian RGBA masks
#define RGBA_R 0xff000000
#define RGBA_G 0x00ff0000
#define RGBA_B 0x0000ff00
#define RGBA_A 0x000000ff

void sdl_paint_png(WhistFrontend* frontend, const char* filename, int output_width,
                   int output_height, int x, int y) {
    SDLFrontendContext* context = frontend->context;
    unsigned int w, h;
    uint8_t* image;

    // Decode to 32-bit RGBA
    unsigned int error = lodepng_decode32_file(&image, &w, &h, filename);
    if (error) {
        LOG_ERROR("Failed to decode PNG: %s", lodepng_error_text(error));
        return;
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image, w, h, sizeof(uint32_t) * 8, sizeof(uint32_t) * w, RGBA_R, RGBA_G, RGBA_B, RGBA_A);
    if (surface == NULL) {
        LOG_ERROR("Failed to create surface from PNG: %s", SDL_GetError());
        free(image);
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    SDL_FreeSurface(surface);
    free(image);
    if (texture == NULL) {
        LOG_ERROR("Failed to create texture from PNG: %s", SDL_GetError());
        return;
    }

    // TODO: Formalize window position constants.
    if (x == -1) {
        // Center horizontally
        x = (output_width - w) / 2;
    }
    if (y == -1) {
        // Place at bottom
        y = output_height - h;
    }
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderCopy(context->renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
}

void sdl_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetWindowFullscreen(context->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void sdl_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetRenderDrawColor(context->renderer, color->red, color->green, color->blue,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context->renderer);
}

// On macOS & Windows, SDL outputs the texture with the last pixel on the bottom and
// right sides without data, rendering it green (NV12 color format). We're not sure
// why that is the case, but in the meantime, clipping that pixel makes the visual
// look seamless.
#if defined(__APPLE__) || defined(_WIN32)
#define CLIPPED_PIXELS 1
#else
#define CLIPPED_PIXELS 0
#endif

void sdl_paint_avframe(WhistFrontend* frontend, AVFrame* frame, int output_width,
                       int output_height) {
    SDLFrontendContext* context = frontend->context;
    // The texture object we allocate is larger than the frame,
    // so we only copy the valid section of the frame into the texture.
    SDL_Rect texture_rect = {
        .x = 0,
        .y = 0,
        .w = frame->width,
        .h = frame->height,
    };

    // Update the SDLTexture with the given nvdata
    int res;
    if (frame->format == AV_PIX_FMT_NV12) {
        // A normal NV12 frame in CPU memory.
        res = SDL_UpdateNVTexture(context->texture, &texture_rect, frame->data[0],
                                  frame->linesize[0], frame->data[1], frame->linesize[1]);
    } else if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
        // A CVPixelBufferRef containing both planes in GPU memory.
        res = SDL_UpdateNVTexture(context->texture, &texture_rect, frame->data[3], frame->width,
                                  frame->data[3], frame->width);
    } else {
        LOG_ERROR("Unsupported frame format: %s", av_get_pix_fmt_name(frame->format));
        return;
    }
    if (res < 0) {
        LOG_ERROR("Failed to update texture: %s", SDL_GetError());
        return;
    }

    // Take the subsection of texture that should be rendered to screen,
    // and draw it on the renderer
    SDL_Rect output_rect = {
        .x = 0,
        .y = 0,
        .w = min(output_width, texture_rect.w) - CLIPPED_PIXELS,
        .h = min(output_height, texture_rect.h) - CLIPPED_PIXELS,
    };
    SDL_RenderCopy(context->renderer, context->texture, &output_rect, NULL);
}

void sdl_render(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    SDL_RenderPresent(context->renderer);
    sdl_native_declare_user_activity();
}

void sdl_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    SDLFrontendContext* context = frontend->context;
    if (cursor == NULL || cursor->hash == context->cursor.hash) {
        return;
    }

    if (cursor->cursor_state != context->cursor.state) {
        if (cursor->cursor_state == CURSOR_STATE_HIDDEN) {
            SDL_GetGlobalMouseState(&context->cursor.last_visible_position.x,
                                    &context->cursor.last_visible_position.y);
            SDL_SetRelativeMouseMode(true);
        } else {
            SDL_SetRelativeMouseMode(false);
            SDL_WarpMouseGlobal(context->cursor.last_visible_position.x,
                                context->cursor.last_visible_position.y);
        }
        context->cursor.state = cursor->cursor_state;
    }

    if (context->cursor.state == CURSOR_STATE_HIDDEN) {
        return;
    }

    SDL_Cursor* sdl_cursor = NULL;
    if (cursor->using_png) {
        uint8_t* rgba = whist_cursor_info_to_rgba(cursor);
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            rgba, cursor->png_width, cursor->png_height, sizeof(uint32_t) * 8,
            sizeof(uint32_t) * cursor->png_width, RGBA_R, RGBA_G, RGBA_B, RGBA_A);
        // This allows for correct blit-resizing.
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

#ifdef _WIN32
        // Windows SDL cursors don't scale to DPI.
        // TODO: https://github.com/libsdl-org/SDL/pull/4927 may change this.
        const int dpi = 96;
#else
        const int dpi = sdl_get_window_dpi(frontend);
#endif  // defined(_WIN32)

        SDL_Surface* scaled = SDL_CreateRGBSurface(
            0, cursor->png_width * 96 / dpi, cursor->png_height * 96 / dpi,
            surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask,
            surface->format->Bmask, surface->format->Amask);

        SDL_BlitScaled(surface, NULL, scaled, NULL);
        SDL_FreeSurface(surface);
        free(rgba);

        // TODO: Consider SDL_SetSurfaceBlendMode here since X11 cursor image
        // is pre-alpha-multiplied.

        sdl_cursor = SDL_CreateColorCursor(scaled, cursor->png_hot_x * 96 / dpi,
                                           cursor->png_hot_y * 96 / dpi);
        SDL_FreeSurface(scaled);
    } else {
        sdl_cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursor->cursor_id);
    }
    SDL_SetCursor(sdl_cursor);

    // SDL_SetCursor requires that the currently set cursor still exists.
    if (context->cursor.handle != NULL) {
        SDL_FreeCursor(context->cursor.handle);
    }
    context->cursor.handle = sdl_cursor;
    context->cursor.hash = cursor->hash;
}
