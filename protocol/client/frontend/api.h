#ifndef WHIST_CLIENT_FRONTEND_API_H
#define WHIST_CLIENT_FRONTEND_API_H

#define FRONTEND_API(GENERATOR)                                                                    \
    GENERATOR(WhistStatus, init, WhistFrontend* frontend, int width, int height,                   \
              const char* title, const WhistRGBColor* color)                                       \
    GENERATOR(void, destroy, WhistFrontend* frontend)                                              \
    GENERATOR(void, open_audio, WhistFrontend* frontend, unsigned int frequency,                   \
              unsigned int channels)                                                               \
    GENERATOR(bool, audio_is_open, WhistFrontend* frontend)                                        \
    GENERATOR(void, close_audio, WhistFrontend* frontend)                                          \
    GENERATOR(WhistStatus, queue_audio, WhistFrontend* frontend, const uint8_t* data, size_t size) \
    GENERATOR(size_t, get_audio_buffer_size, WhistFrontend* frontend)                              \
    GENERATOR(WhistStatus, get_window_pixel_size, WhistFrontend* frontend, int id, int* width,     \
              int* height)                                                                         \
    GENERATOR(WhistStatus, get_window_virtual_size, WhistFrontend* frontend, int id, int* width,   \
              int* height)                                                                         \
    GENERATOR(WhistStatus, get_window_display_index, WhistFrontend* frontend, int* index)          \
    GENERATOR(int, get_window_dpi, WhistFrontend* frontend)                                        \
    GENERATOR(bool, is_any_window_visible, WhistFrontend* frontend)                                \
    GENERATOR(WhistStatus, set_title, WhistFrontend* frontend, int id, const char* title)          \
    GENERATOR(void, restore_window, WhistFrontend* frontend, int id)                               \
    GENERATOR(void, set_window_fullscreen, WhistFrontend* frontend, int id, bool fullscreen)       \
    GENERATOR(void, resize_window, WhistFrontend* frontend, int id, int width, int height)         \
    GENERATOR(bool, poll_event, WhistFrontend* frontend, WhistFrontendEvent* event)                \
    GENERATOR(bool, wait_event, WhistFrontend* frontend, WhistFrontendEvent* event,                \
              int timeout_ms)                                                                      \
    GENERATOR(void, interrupt, WhistFrontend* frontend)                                            \
    GENERATOR(const char*, get_chosen_file, WhistFrontend* frontend)                               \
    GENERATOR(void*, file_download_start, WhistFrontend* frontend, const char* file_path,          \
              int64_t file_size)                                                                   \
    GENERATOR(void, file_download_update, WhistFrontend* frontend, void* opaque,                   \
              int64_t bytes_so_far, int64_t bytes_per_sec)                                         \
    GENERATOR(void, file_download_complete, WhistFrontend* frontend, void* opaque)                 \
    GENERATOR(void, set_cursor, WhistFrontend* frontend, WhistCursorInfo* cursor)                  \
    GENERATOR(void, get_keyboard_state, WhistFrontend* frontend, const uint8_t** key_state,        \
              int* key_count, int* mod_state)                                                      \
    GENERATOR(void, get_video_device, WhistFrontend* frontend, AVBufferRef** device,               \
              enum AVPixelFormat* format)                                                          \
    GENERATOR(WhistStatus, update_video, WhistFrontend* frontend, AVFrame* frame,                  \
              WhistWindow* window_data, int num_windows)                                           \
    GENERATOR(void, paint_png, WhistFrontend* frontend, const uint8_t* data, size_t data_size,     \
              int x, int y)                                                                        \
    GENERATOR(void, paint_video, WhistFrontend* frontend)                                          \
    GENERATOR(void, paint_solid, WhistFrontend* frontend, const WhistRGBColor* color)              \
    GENERATOR(void, render, WhistFrontend* frontend)                                               \
    GENERATOR(void, set_titlebar_color, WhistFrontend* frontend, int id,                           \
              const WhistRGBColor* color)                                                          \
    GENERATOR(void, display_notification, WhistFrontend* frontend, const WhistNotification* notif) \
    GENERATOR(void, declare_user_activity, WhistFrontend* frontend)

#endif  // WHIST_CLIENT_FRONTEND_API_H
