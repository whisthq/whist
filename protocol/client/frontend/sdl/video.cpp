#include <SDL2/SDL_video.h>
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

void sdl_paint_png(WhistFrontend* frontend, const uint8_t* data, size_t data_size, int x, int y) {
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
                x = (context->latest_pixel_width - w) / 2;
            }
            if (y == -1) {
                // Place at bottom
                y = context->latest_pixel_height - h;
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
#if !USING_MULTIWINDOW
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
#endif
}

void sdl_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

    for (const auto& [window_id, window_context] : context->windows) {
        SDL_SetRenderDrawColor(window_context->renderer, color->red, color->green, color->blue,
                               SDL_ALPHA_OPAQUE);
        SDL_RenderClear(window_context->renderer);
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

WhistStatus sdl_update_video(WhistFrontend* frontend, AVFrame* frame, WhistWindow* window_data,
                             int num_windows) {
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

#if USING_MULTIWINDOW
    // Loop over the Frame's windows list, and create any uncreated windows
    int dpi_scale = sdl_get_dpi_scale(frontend);
    for (int i = 0; i < num_windows; i++) {
        int id = window_data[i].id;
        // If that window doesn't exist yet,
        if (!context->windows.contains(id)) {
            LOG_INFO("Creating window with ID %d", id);
            // Create and populate the window_context
            SDLWindowContext* window_context = new SDLWindowContext();
            window_context->to_be_created = true;
            window_context->window_id = id;
            window_context->x = window_data[i].x;
            window_context->y = window_data[i].y;
            window_context->width = window_data[i].width;
            window_context->height = window_data[i].height;
            window_context->title = "Default Title";
            window_context->color = window_data[i].corner_color;
            window_context->is_fullscreen = window_data[i].is_fullscreen;
            window_context->is_resizable = false;

            // Then create the actual SDL window
            context->windows[id] = window_context;
            sdl_create_window(frontend, id);
        } else {
            // Else, update the existant window context
            SDLWindowContext* window_context = context->windows[id];
            // Update fullscreen
            window_context->is_fullscreen =
                SDL_GetWindowFlags(window_context->window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
            if (window_context->is_fullscreen != window_data[i].is_fullscreen) {
                SDL_SetWindowFullscreen(window_context->window, window_data[i].is_fullscreen
                                                                    ? SDL_WINDOW_FULLSCREEN_DESKTOP
                                                                    : 0);
            }
            // Update SDL x/y/w/h, in pixel coordinates
            SDL_GL_GetDrawableSize(window_context->window, &window_context->width,
                                   &window_context->height);
            if (window_context->width != window_data[i].width ||
                window_context->height != window_data[i].height) {
                // Set window size, adjusting for DPI scale
                SDL_SetWindowSize(window_context->window, window_data[i].width / dpi_scale,
                                  window_data[i].height / dpi_scale);
            }
            SDL_GetWindowPosition(window_context->window, &window_context->x, &window_context->y);
            window_context->y -= Y_SHIFT;
            window_context->x *= dpi_scale;
            window_context->y *= dpi_scale;
            if (window_context->x != window_data[i].x || window_context->y != window_data[i].y) {
                // Set the window position, adjusting for DPI scale and y-shift
                SDL_SetWindowPosition(window_context->window, window_data[i].x / dpi_scale,
                                      window_data[i].y / dpi_scale + Y_SHIFT);
            }
            window_context->x = window_data[i].x;
            window_context->y = window_data[i].y;
            window_context->width = window_data[i].width;
            window_context->height = window_data[i].height;
            window_context->title = "Default Title";
            window_context->color = window_data[i].corner_color;
            window_context->is_fullscreen = window_data[i].is_fullscreen;
            window_context->is_resizable = false;
        }
    }
    // Get IDs that are in context->windows, but not in the most recent Frame's window list
    vector<int> remove_ids;
    for (const auto& [window_id, window_context] : context->windows) {
        bool found = false;
        for (int i = 0; i < num_windows; i++) {
            // TODO: Fix uint64_t usage
            if ((int)window_data[i].id == window_id) {
                found = true;
                break;
            }
        }
        // If that SDL window isn't in the Frame's window list, get ready to destroy it
        if (!found) {
            remove_ids.push_back(window_id);
        }
    }

    // Destroy those IDs
    for (int remove_id : remove_ids) {
        LOG_INFO("Destroying window with ID %d", remove_id);
        sdl_destroy_window(frontend, remove_id);
    }
#else
    SDLWindowContext* root_window_context = context->windows[0];
    // Populate with whole frame, for single-window mode
    root_window_context->x = 0;
    root_window_context->y = 0;
    root_window_context->width = frame->width;
    root_window_context->height = frame->height;
#endif

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
                sdl_d3d11_wait(window_context);
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
                window_context->texture = sdl_d3d11_create_texture(window_context, frame);
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

void sdl_paint_video(WhistFrontend* frontend) {
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

#if USING_MULTIWINDOW
        window_context->sdl_width = MAX_SCREEN_WIDTH;
        window_context->sdl_height = MAX_SCREEN_HEIGHT;
#else
        window_context->sdl_width = output_width;
        window_context->sdl_height = output_height;
#endif
        // Take that crop when rendering it out
        SDL_Rect src_rect = {
            .x = window_context->x,
            .y = window_context->y,
            .w = min(window_context->sdl_width, window_context->width),
            .h = min(window_context->sdl_height, window_context->height),
        };
        res = SDL_RenderCopy(window_context->renderer, window_context->texture, &src_rect, NULL);
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

static SDL_SystemCursor sdl_system_cursor_from_whist_cursor_type(WhistCursorType type) {
    FATAL_ASSERT(type != WHIST_CURSOR_PNG);
    switch (type) {
        case WHIST_CURSOR_CROSSHAIR: {
            return SDL_SYSTEM_CURSOR_CROSSHAIR;
        }
        case WHIST_CURSOR_HAND: {
            return SDL_SYSTEM_CURSOR_HAND;
        }
        case WHIST_CURSOR_IBEAM: {
            return SDL_SYSTEM_CURSOR_IBEAM;
        }
        case WHIST_CURSOR_NO_DROP:
        case WHIST_CURSOR_NOT_ALLOWED: {
            return SDL_SYSTEM_CURSOR_NO;
        }
        case WHIST_CURSOR_RESIZE_NE:
        case WHIST_CURSOR_RESIZE_NESW:
        case WHIST_CURSOR_RESIZE_SW: {
            return SDL_SYSTEM_CURSOR_SIZENESW;
        }
        case WHIST_CURSOR_RESIZE_NS:
        case WHIST_CURSOR_RESIZE_ROW: {
            return SDL_SYSTEM_CURSOR_SIZENS;
        }
        case WHIST_CURSOR_RESIZE_NW:
        case WHIST_CURSOR_RESIZE_NWSE:
        case WHIST_CURSOR_RESIZE_SE: {
            return SDL_SYSTEM_CURSOR_SIZENWSE;
        }
        case WHIST_CURSOR_RESIZE_COL:
        case WHIST_CURSOR_RESIZE_EW: {
            return SDL_SYSTEM_CURSOR_SIZEWE;
        }
        case WHIST_CURSOR_WAIT: {
            return SDL_SYSTEM_CURSOR_WAIT;
        }
        case WHIST_CURSOR_PROGRESS: {
            return SDL_SYSTEM_CURSOR_WAITARROW;
        }
        default: {
            // There are three cases.
            // 1. WHIST_CURSOR_ARROW => this is correct
            // 2. WHIST_CURSOR_NONE => anything is fine; we hid the cursor
            // 3. This is a cursor SDL can't produce, so we'll just use the default.
            //    Note that this is technically a loss in functionality for the SDL
            //    frontend, as we can't fallback to PNGs that aren't transmitted in
            //    this case! Consider it a temporary sacrifice for virtual/Chromium.
            // TODO: Fix 3.
            return SDL_SYSTEM_CURSOR_ARROW;
        }
    }
}

void sdl_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (cursor == NULL || cursor->hash == context->cursor.hash) {
        return;
    }

    if (cursor->mode != context->cursor.mode) {
        if (cursor->mode == MOUSE_MODE_RELATIVE) {
            SDL_GetGlobalMouseState(&context->cursor.last_visible_position.x,
                                    &context->cursor.last_visible_position.y);
            SDL_SetRelativeMouseMode((SDL_bool) true);
        } else {
            SDL_SetRelativeMouseMode((SDL_bool) false);
            SDL_WarpMouseGlobal(context->cursor.last_visible_position.x,
                                context->cursor.last_visible_position.y);
        }
        context->cursor.mode = cursor->mode;
    }

    if (context->cursor.mode == MOUSE_MODE_RELATIVE) {
        return;
    }

    // TODO: If this is too heavy, we can cache what SDL_ShowCursor(SDL_QUERY) returns, and only
    // update if it needs to change.
    if (cursor->type == WHIST_CURSOR_NONE) {
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_ShowCursor(SDL_ENABLE);
    }

    SDL_Cursor* sdl_cursor = NULL;
    if (cursor->type == WHIST_CURSOR_PNG) {
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
        sdl_cursor = SDL_CreateSystemCursor(sdl_system_cursor_from_whist_cursor_type(cursor->type));
    }
    SDL_SetCursor(sdl_cursor);

    // SDL_SetCursor requires that the currently set cursor still exists.
    if (context->cursor.handle != NULL) {
        SDL_FreeCursor(context->cursor.handle);
    }
    context->cursor.handle = sdl_cursor;
    context->cursor.hash = cursor->hash;
}
