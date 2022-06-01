#include <whist/core/whist.h>
extern "C" {
#include "common.h"
#include "native.h"
// Load lodepng with c linkage
#define LODEPNG_NO_COMPILE_CPP
#include <whist/utils/lodepng.h>
}
#include "sdl_struct.hpp"

// Little-endian RGBA masks
#define RGBA_R 0xff000000
#define RGBA_G 0x00ff0000
#define RGBA_B 0x0000ff00
#define RGBA_A 0x000000ff

void sdl_paint_png(WhistFrontend* frontend, const uint8_t* data, size_t data_size, int output_width,
                   int output_height, int x, int y) {
    /*
     * Render a PNG to all windows in the frontend at position x/y/output_width/output_height
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    unsigned int w, h;
    uint8_t* image;

    // Decode to 32-bit RGBA
    unsigned int error = lodepng_decode32(&image, &w, &h, data, data_size);
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
    for (const auto& [window_id, window_context] : context->windows) {
        // only show PNG if it's smaller than the window
        if (((int)w <= window_context->sdl_width) && ((int)h <= window_context->sdl_height)) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(window_context->renderer, surface);
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
            SDL_Rect rect = {x, y, (int)w, (int)h};
            SDL_RenderCopy(window_context->renderer, texture, NULL, &rect);

            SDL_DestroyTexture(texture);
        }
    }
    SDL_FreeSurface(surface);
    free(image);
}

void sdl_set_window_fullscreen(WhistFrontend* frontend, int id, bool fullscreen) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .timestamp = 0,
                .code = SDL_FRONTEND_EVENT_FULLSCREEN,
                // Cast the data directly into the 8bytes of void*,
                // So it holds the data, rather than an actual pointer
                .data1 = (void*)(intptr_t)fullscreen,
                .data2 = (void*)(intptr_t)id,
            },
    };
    SDL_PushEvent(&event);
}

void sdl_paint_solid(WhistFrontend* frontend, int id, const WhistRGBColor* color) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

    if (context->windows.contains(id)) {
        SDLWindowContext* window_context = context->windows[id];
        SDL_SetRenderDrawColor(window_context->renderer, color->red, color->green, color->blue,
                               SDL_ALPHA_OPAQUE);
        SDL_RenderClear(window_context->renderer);
    } else {
        LOG_FATAL("Tried to paint window %d, but no such window exists!", id);
    }
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
    /*
     * Copy/upload the video frame texture sent by the server to each window in the frontend.
     * We currently upload that one texture multiple times, but will transition to more efficient
     * ways of copying the texture between renderers.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    int res;

    SDL_PixelFormatEnum format = sdl_get_pixel_format((AVPixelFormat)frame->format);
    if (format == SDL_PIXELFORMAT_UNKNOWN) {
        LOG_ERROR("Invalid pixel format %s given to SDL renderer.",
                  av_get_pix_fmt_name((AVPixelFormat)frame->format));
        return WHIST_ERROR_INVALID_ARGUMENT;
    }

    // Formats we support for texture import to SDL.
    bool import_texture =
        (frame->format == AV_PIX_FMT_VIDEOTOOLBOX || frame->format == AV_PIX_FMT_D3D11);

    // TODO: for now I've wrapped the logic in a loop, but we should only be importing the first
    // texture and blitting the rest
    for (const auto& [window_id, window_context] : context->windows) {
        if (import_texture || format != window_context->texture_format ||
            frame->width != context->video.frame_width ||
            frame->height != context->video.frame_height) {
            // When importing we will make a new SDL texture referring to
            // the imported one, so the old texture should be destroyed.
            // When uploading this is only needed if the format or
            // dimensions change.
            if (window_context->texture) {
                SDL_DestroyTexture(window_context->texture);
                window_context->texture = NULL;
            }
#if OS_IS(OS_WIN32)
            if (import_texture) {
                // The D3D11 SDL render path is entirely asynchronous, with
                // no synchronisation points at all.  Since video decode
                // happens on another thread we need to ensure that the
                // render involving the previous frame has actually
                // completed before we free it, since that will allow the
                // texture to be reused by the decoder.
                sdl_d3d11_wait(context);
            }
#endif  // OS_IS(OS_WIN32)
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

                window_context->texture = SDL_CreateTextureFromHandle(
                    window_context->renderer, format, SDL_TEXTUREACCESS_STATIC, frame->width,
                    frame->height, handle);
                if (window_context->texture == NULL) {
                    LOG_ERROR("Failed to import Core Video texture: %s.", SDL_GetError());
                    return WHIST_ERROR_EXTERNAL;
                }
            } else if (frame->format == AV_PIX_FMT_D3D11) {
#if OS_IS(OS_WIN32)
                window_context->texture =
                    sdl_d3d11_create_texture(context, window_context->renderer, frame);
                if (window_context->texture == NULL) {
                    LOG_ERROR("Failed to import D3D11 texture.");
                    return WHIST_ERROR_EXTERNAL;
                }
#else
                LOG_FATAL("D3D11 support requires Windows.");
#endif
            }
        } else {
            if (window_context->texture == NULL) {
                // Create new video texture.
                window_context->texture =
                    SDL_CreateTexture(window_context->renderer, format, SDL_TEXTUREACCESS_STREAMING,
                                      frame->width, frame->height);
                if (window_context->texture == NULL) {
                    LOG_ERROR("Failed to create %s video texture: %s.",
                              av_get_pix_fmt_name((AVPixelFormat)frame->format), SDL_GetError());
                    return WHIST_ERROR_EXTERNAL;
                }

                LOG_INFO("Using %s video texture.",
                         av_get_pix_fmt_name((AVPixelFormat)frame->format));
                window_context->texture_format = format;
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
                res = SDL_UpdateNVTexture(window_context->texture, &texture_rect, frame->data[0],
                                          frame->linesize[0], frame->data[1], frame->linesize[1]);
            } else if (frame->format == AV_PIX_FMT_YUV420P) {
                res = SDL_UpdateYUVTexture(window_context->texture, &texture_rect, frame->data[0],
                                           frame->linesize[0], frame->data[1], frame->linesize[1],
                                           frame->data[2], frame->linesize[2]);
            } else {
                LOG_FATAL("Invalid format %s for texture update.",
                          av_get_pix_fmt_name((AVPixelFormat)frame->format));
            }

            if (res < 0) {
                LOG_ERROR("Failed to update texture from %s frame: %s.",
                          av_get_pix_fmt_name((AVPixelFormat)frame->format), SDL_GetError());
                return WHIST_ERROR_EXTERNAL;
            }
        }
    }
    context->video.frame_width = frame->width;
    context->video.frame_height = frame->height;

    return WHIST_SUCCESS;
}

void sdl_paint_video(WhistFrontend* frontend, int output_width, int output_height) {
    /*
     * Copy the appropiate portion of the video frame to each window's renderer texture. Each window
     * crops only the rectangle determined by its window context's x/y/width/height.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    int res;

    for (const auto& [window_id, window_context] : context->windows) {
        if (window_context->texture == NULL) {
            // No texture to render - this can happen at startup if no video
            // has been decoded yet.  Do nothing here, since the screen was
            // cleared to a solid colour anyway.
            continue;
        }

        // Populate with whole frame, for single-window mode
        window_context->x = 0;
        window_context->y = 0;
        window_context->width = context->video.frame_width;
        window_context->height = context->video.frame_height;
        window_context->sdl_width = output_width;
        window_context->sdl_height = output_height;
        // Take that crop when rendering it out
        SDL_Rect output_rect = {
            .x = window_context->x,
            .y = window_context->y,
            .w = min(window_context->sdl_width, window_context->width),
            .h = min(window_context->sdl_height, window_context->height),
        };
        res = SDL_RenderCopy(window_context->renderer, window_context->texture, &output_rect, NULL);
        if (res < 0) {
            LOG_WARNING("Failed to render texture: %s.", SDL_GetError());
        }

        if (!window_context->video_has_rendered) {
            window_context->video_has_rendered = true;
        }
    }
}

void sdl_render(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

    for (const auto& [window_id, window_context] : context->windows) {
        SDL_RenderPresent(window_context->renderer);

        // TODO: necessary if we only show windows on demand?
        if (window_context->video_has_rendered && !window_context->window_has_shown) {
            SDL_ShowWindow(window_context->window);
            window_context->window_has_shown = true;
        }
    }
}

void sdl_declare_user_activity(WhistFrontend* frontend) { sdl_native_declare_user_activity(); }

void sdl_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (cursor == NULL || cursor->hash == context->cursor.hash) {
        return;
    }

    if (cursor->cursor_state != context->cursor.state) {
        if (cursor->cursor_state == CURSOR_STATE_HIDDEN) {
            SDL_GetGlobalMouseState(&context->cursor.last_visible_position.x,
                                    &context->cursor.last_visible_position.y);
            SDL_SetRelativeMouseMode((SDL_bool) true);
        } else {
            SDL_SetRelativeMouseMode((SDL_bool) false);
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

#if OS_IS(OS_WIN32)
        // Windows SDL cursors don't scale to DPI.
        // TODO: https://github.com/libsdl-org/SDL/pull/4927 may change this.
        const int dpi = 96;
#else
        const int dpi = sdl_get_window_dpi(frontend);
#endif  // Windows

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
