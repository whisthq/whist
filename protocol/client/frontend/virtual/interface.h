#ifndef INTERFACE_H
#define INTERFACE_H

#include "../frontend_structs.h"
#include "../../whist_client.h"
#include "../../../whist/utils/os_utils.h"

typedef enum ModifierKeyFlags {
    SHIFT = 1 << 0,
    CTRL = 1 << 1,
    ALT = 1 << 2,
    GUI = 1 << 3,
    CAPS = 1 << 4
} ModifierKeyFlags;

typedef const char* (*OnFileUploadCallback)(void* data);
typedef void (*OnCursorChangeCallback)(void* data, const char* cursor, bool relative_mouse_mode);
typedef void* (*OnFileDownloadStart)(const char* file_path, int64_t file_size);
typedef void (*OnFileDownloadUpdate)(void* opaque, int64_t bytes_so_far, int64_t bytes_per_sec);
typedef void (*OnFileDownloadComplete)(void* opaque);
typedef void (*OnNotificationCallback)(const WhistNotification* notif);
typedef void (*VideoFrameCallback)(int window_id, void* frame_ref);
typedef int (*GetModifierKeyState)(void);

typedef struct VirtualInterface {
    struct {
        void (*connect)(void);
        int (*create_window)(void);
        void (*register_context)(int window_id, void* context);
        void (*destroy_window)(int window_id);
        int (*start)(int argc, const char* argv[]);
        void (*disconnect)(void);
    } lifecycle;
    struct {
        void* (*get_frame_ref)(void);
        void* (*get_handle_from_frame_ref)(void* frame_ref);
        void (*get_frame_ref_yuv_data)(void* frame_ref, uint8_t*** data, int** linesize, int* width,
                                       int* height, int* visible_width, int* visible_height);
        void (*free_frame_ref)(void* frame_ref);
        void (*set_on_cursor_change_callback)(int window_id, OnCursorChangeCallback cb);
        void (*set_video_frame_callback)(int window_id, VideoFrameCallback cb);
        void (*set_video_playing)(int window_id, bool playing);
    } video;
    struct {
        void (*send)(const WhistFrontendEvent* event);
        void (*set_get_modifier_key_state)(GetModifierKeyState cb);
    } events;
    struct {
        void (*set_on_file_upload_callback)(int window_id, OnFileUploadCallback cb);
        void (*set_on_file_download_start_callback)(OnFileDownloadStart cb);
        void (*set_on_file_download_update_callback)(OnFileDownloadUpdate cb);
        void (*set_on_file_download_complete_callback)(OnFileDownloadComplete cb);
        // TODO: Move this function out of the `file` module.
        void (*set_on_notification_callback)(OnNotificationCallback cb);
    } file;
} VirtualInterface;

const VirtualInterface* get_virtual_interface(void);

// This is a crutch. Once video is callback-ized we won't need it anymore.
#if FROM_WHIST_PROTOCOL
void virtual_interface_send_frame(AVFrame* frame);
#endif  // FROM_WHIST_PROTOCOL

#endif  // INTERFACE_H
