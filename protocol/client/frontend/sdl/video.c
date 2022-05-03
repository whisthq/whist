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

static SDL_PixelFormatEnum sdl_get_pixel_format(enum AVPixelFormat pixfmt) {
    switch (pixfmt) {
        case AV_PIX_FMT_NV12:
        case AV_PIX_FMT_D3D11:
        case AV_PIX_FMT_VIDEOTOOLBOX:
            // Hardware formats all use NV12 internally.
            return SDL_PIXELFORMAT_NV12;
        case AV_PIX_FMT_YUV420P:
            return SDL_PIXELFORMAT_IYUV;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

WhistStatus sdl_update_video(WhistFrontend* frontend, AVFrame* frame) {
    SDLFrontendContext* context = frontend->context;
    int res;

    SDL_PixelFormatEnum format = sdl_get_pixel_format(frame->format);
    if (format == SDL_PIXELFORMAT_UNKNOWN) {
        LOG_ERROR("Invalid pixel format %s given to SDL renderer.",
                  av_get_pix_fmt_name(frame->format));
        return WHIST_ERROR_INVALID_ARGUMENT;
    }

    // Formats we support for texture import to SDL.
    bool import_texture =
        (frame->format == AV_PIX_FMT_VIDEOTOOLBOX || frame->format == AV_PIX_FMT_D3D11);

    if (import_texture || format != context->video.texture_format) {
        // When importing we will make a new SDL texture referring to
        // the imported one, so the old texture should be destroyed.
        // When uploading this is only needed if the format changes.
        if (context->video.texture) {
            SDL_DestroyTexture(context->video.texture);
            context->video.texture = NULL;
        }
#ifdef _WIN32
        if (import_texture) {
            // The D3D11 SDL render path is entirely asynchronous, with
            // no synchronisation points at all.  Since video decode
            // happens on another thread we need to ensure that the
            // render involving the previous frame has actually
            // completed before we free it, since that will allow the
            // texture to be reused by the decoder.
            sdl_d3d11_wait(context);
        }
#endif
        av_frame_unref(context->video.frame_reference);
    }

    if (import_texture) {
        if (context->video.frame_reference == NULL) {
            context->video.frame_reference = av_frame_alloc();
        }
        av_frame_ref(context->video.frame_reference, frame);

        if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
            // This is a pointer to the CVPixelBuffer containing the
            // frame data in GPU memory.
            void* handle = frame->data[3];

            context->video.texture =
                SDL_CreateTextureFromHandle(context->renderer, format, SDL_TEXTUREACCESS_STATIC,
                                            frame->width, frame->height, handle);
            if (context->video.texture == NULL) {
                LOG_ERROR("Failed to import Core Video texture: %s.", SDL_GetError());
                return WHIST_ERROR_EXTERNAL;
            }
        } else if (frame->format == AV_PIX_FMT_D3D11) {
#ifdef _WIN32
            context->video.texture = sdl_d3d11_create_texture(context, frame);
            if (context->video.texture == NULL) {
                LOG_ERROR("Failed to import D3D11 texture.");
                return WHIST_ERROR_EXTERNAL;
            }
#else
            LOG_FATAL("D3D11 support requires Windows.");
#endif
        }
    } else {
        if (context->video.texture == NULL) {
            // Create new video texture.
            context->video.texture =
                SDL_CreateTexture(context->renderer, format, SDL_TEXTUREACCESS_STREAMING,
                                  MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
            if (context->video.texture == NULL) {
                LOG_ERROR("Failed to create %s video texture: %s.",
                          av_get_pix_fmt_name(frame->format), SDL_GetError());
                return WHIST_ERROR_EXTERNAL;
            }

            LOG_INFO("Using %s video texture.", av_get_pix_fmt_name(frame->format));
            context->video.texture_format = format;
        }

        // The texture object we allocate is larger than the frame, so
        // we only copy the valid section of the frame into the texture.
        SDL_Rect texture_rect = {
            .x = 0,
            .y = 0,
            .w = frame->width,
            .h = frame->height,
        };
        if (frame->format == AV_PIX_FMT_NV12) {
            res = SDL_UpdateNVTexture(context->video.texture, &texture_rect, frame->data[0],
                                      frame->linesize[0], frame->data[1], frame->linesize[1]);
        } else if (frame->format == AV_PIX_FMT_YUV420P) {
            res = SDL_UpdateYUVTexture(context->video.texture, &texture_rect, frame->data[0],
                                       frame->linesize[0], frame->data[1], frame->linesize[1],
                                       frame->data[2], frame->linesize[2]);
        } else {
            LOG_FATAL("Invalid format %s for texture update.", av_get_pix_fmt_name(frame->format));
        }

        if (res < 0) {
            LOG_ERROR("Failed to update texture from %s frame: %s.",
                      av_get_pix_fmt_name(frame->format), SDL_GetError());
            return WHIST_ERROR_EXTERNAL;
        }
    }

    context->video.frame_width = frame->width;
    context->video.frame_height = frame->height;

    return WHIST_SUCCESS;
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

void sdl_paint_video(WhistFrontend* frontend, int output_x, int output_y, int output_width, int output_height) {
    SDLFrontendContext* context = frontend->context;
    int res;

    if (context->video.texture == NULL) {
        // No texture to render - this can happen at startup if no video
        // has been decoded yet.  Do nothing here, since the screen was
        // cleared to a solid colour anyway.
        return;
    }

    // Take the subsection of texture that should be rendered to screen,
    // and draw it on the renderer
    SDL_Rect output_rect = {
        .x = output_x,
        .y = output_y,
        .w = min(output_width, context->video.frame_width) - CLIPPED_PIXELS,
        .h = min(output_height, context->video.frame_height) - CLIPPED_PIXELS,
    };
    res = SDL_RenderCopy(context->renderer, context->video.texture, &output_rect, NULL);
    if (res < 0) {
        LOG_WARNING("Failed to render texture: %s.", SDL_GetError());
    }

    if (!context->video_has_rendered) {
        context->video_has_rendered = true;
    }
}

void sdl_render(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;

    SDL_RenderPresent(context->renderer);

    if (context->video_has_rendered && !context->window_has_shown) {
        SDL_ShowWindow(context->window);
        context->window_has_shown = true;
    }
}

void sdl_declare_user_activity(WhistFrontend* frontend) { sdl_native_declare_user_activity(); }

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
