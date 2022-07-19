#include "common.h"
// This is a crutch. Once video is callback-ized we won't need it anymore.
#define FROM_WHIST_PROTOCOL true
#include "interface.h"
#include <whist/utils/queue.h>
#include <whist/network/network_algorithm.h>

// Just chosen a very large number for events queue size. If required we can optimize/reduce it.
#define MAX_EVENTS_QUEUED 10000

QueueContext* events_queue = NULL;

void* callback_context = NULL;
OnCursorChangeCallback on_cursor_change = NULL;
OnFileUploadCallback on_file_upload = NULL;
OnFileDownloadStart on_file_download_start = NULL;
OnFileDownloadUpdate on_file_download_update = NULL;
OnFileDownloadComplete on_file_download_complete = NULL;
static VideoFrameCallback video_frame_callback_ptr = NULL;

static WhistMutex lock;
static AVFrame* pending = NULL;
static bool connected = false;
static int requested_width;
static int requested_height;

static void virtual_interface_connect(void) {
    lock = whist_create_mutex();
    events_queue = fifo_queue_create(sizeof(WhistFrontendEvent), MAX_EVENTS_QUEUED);
    connected = true;
}

static void virtual_interface_set_on_cursor_change_callback(OnCursorChangeCallback cb) {
    on_cursor_change = cb;
}

static void* virtual_interface_get_frame_ref(void) {
    whist_lock_mutex(lock);
    // Ensure that the interface isn't in callback mode
    FATAL_ASSERT(video_frame_callback_ptr == NULL);
    // Consume the pending AVFrame, and return it
    void* out = pending;
    pending = NULL;
    whist_unlock_mutex(lock);
    return out;
}

static void* virtual_interface_get_handle_from_frame_ref(void* frame_ref) {
    AVFrame* frame = frame_ref;
    // Assuming we want CVPixelBufferRef for now
    return frame->data[3];
}

static void virtual_interface_get_frame_ref_yuv_data(void* frame_ref, uint8_t*** data,
                                                     int** linesize, int* width, int* height,
                                                     int* visible_width, int* visible_height) {
    AVFrame* frame = frame_ref;
    *data = frame->data;
    *linesize = frame->linesize;
    *width = frame->width;
    *height = frame->height;
    // If video width is rounded to the nearest even number, then crop the last pixel
    if (frame->width - requested_width == 1) {
        *visible_width = requested_width;
    } else {
        *visible_width = frame->width;
    }
    // If video height is rounded to the nearest even number, then crop the last pixel
    if (frame->height - requested_height == 1) {
        *visible_height = requested_height;
    } else {
        *visible_height = frame->height;
    }
}

static void virtual_interface_free_frame_ref(void* frame_ref) {
    AVFrame* frame = frame_ref;
    av_frame_free(&frame);
}

void virtual_interface_send_frame(AVFrame* frame) {
    if (!connected) return;

    whist_lock_mutex(lock);
    if (video_frame_callback_ptr != NULL) {
        // If we have a callback, pass into the callback
        AVFrame* new_frame = av_frame_clone(frame);
        video_frame_callback_ptr((void*)new_frame);
    } else {
        // Otherwise, keep the frame as pending
        av_frame_free(&pending);
        pending = av_frame_clone(frame);
    }
    whist_unlock_mutex(lock);
}

static void virtual_interface_set_video_frame_callback(VideoFrameCallback callback_ptr) {
    whist_lock_mutex(lock);
    // If we're newly setting this callback, free any pending frames
    if (video_frame_callback_ptr == NULL) {
        av_frame_free(&pending);
    }
    video_frame_callback_ptr = callback_ptr;
    whist_unlock_mutex(lock);
}

static void virtual_interface_disconnect(void) {
    connected = false;
    whist_destroy_mutex(lock);
    fifo_queue_destroy(events_queue);
}

static void virtual_interface_send_event(const WhistFrontendEvent* frontend_event) {
    if (frontend_event->type == FRONTEND_EVENT_RESIZE) {
        requested_width = frontend_event->resize.width;
        requested_height = frontend_event->resize.height;
    }
    if (fifo_queue_enqueue_item(events_queue, frontend_event) != 0) {
        LOG_ERROR("Virtual event queuing failed");
    }
}

static void virtual_interface_set_on_file_upload_callback(OnFileUploadCallback cb) {
    on_file_upload = cb;
}

static void virtual_interface_set_on_file_download_start_callback(OnFileDownloadStart cb) {
    on_file_download_start = cb;
}

static void virtual_interface_set_on_file_download_update_callback(OnFileDownloadUpdate cb) {
    on_file_download_update = cb;
}

static void virtual_interface_set_on_file_download_complete_callback(OnFileDownloadComplete cb) {
    on_file_download_complete = cb;
}

static void virtual_interface_set_callback_context(void* context) { callback_context = context; }

static const VirtualInterface vi = {
    .lifecycle =
        {
            .connect = virtual_interface_connect,
            .set_callback_context = virtual_interface_set_callback_context,
            .start = whist_client_main,
            .disconnect = virtual_interface_disconnect,
        },
    .video =
        {
            .get_frame_ref = virtual_interface_get_frame_ref,
            .get_handle_from_frame_ref = virtual_interface_get_handle_from_frame_ref,
            .get_frame_ref_yuv_data = virtual_interface_get_frame_ref_yuv_data,
            .free_frame_ref = virtual_interface_free_frame_ref,
            .set_on_cursor_change_callback = virtual_interface_set_on_cursor_change_callback,
            .set_video_frame_callback = virtual_interface_set_video_frame_callback,
        },
    .events =
        {
            .send = virtual_interface_send_event,
        },
    .file =
        {
            .set_on_file_upload_callback = virtual_interface_set_on_file_upload_callback,
            .set_on_file_download_start_callback =
                virtual_interface_set_on_file_download_start_callback,
            .set_on_file_download_update_callback =
                virtual_interface_set_on_file_download_update_callback,
            .set_on_file_download_complete_callback =
                virtual_interface_set_on_file_download_complete_callback,
        },
};

const VirtualInterface* get_virtual_interface(void) { return &vi; }
