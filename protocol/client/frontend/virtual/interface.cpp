#include <whist/core/whist.h>
#include <mutex>
extern "C" {
#include "common.h"
// This is a crutch. Once video is callback-ized we won't need it anymore.
#define FROM_WHIST_PROTOCOL true
#include "interface.h"
#include <whist/utils/queue.h>
#include <whist/network/network_algorithm.h>
}

// Just chosen a very large number for events queue size. If required we can optimize/reduce it.
#define MAX_EVENTS_QUEUED 10000

QueueContext* events_queue = NULL;

void* callback_context = NULL;
OnCursorChangeCallback on_cursor_change = NULL;
OnFileUploadCallback on_file_upload = NULL;
OnFileDownloadStart on_file_download_start = NULL;
OnFileDownloadUpdate on_file_download_update = NULL;
OnFileDownloadComplete on_file_download_complete = NULL;

static WhistMutex lock;
static AVFrame* pending = NULL;
static bool connected = false;
static int requested_width;
static int requested_height;

// Whist window management
struct WhistWindowInformation {
    VideoFrameCallback video_frame_callback_ptr;
};
static std::mutex whist_window_mutex;
static std::map<int, WhistWindowInformation> whist_windows;

static void virtual_interface_connect(void) {
    lock = whist_create_mutex();
    events_queue = fifo_queue_create(sizeof(WhistFrontendEvent), MAX_EVENTS_QUEUED);
    connected = true;
}

static void virtual_interface_set_on_cursor_change_callback(OnCursorChangeCallback cb) {
    on_cursor_change = cb;
}

static void* virtual_interface_get_frame_ref(void) {
    std::lock_guard<std::mutex> guard(whist_window_mutex);
    // Consume the pending AVFrame, and return it
    void* frame_ref = pending;
    pending = NULL;
    return frame_ref;
}

static void* virtual_interface_get_handle_from_frame_ref(void* frame_ref) {
    AVFrame* frame = (AVFrame*)frame_ref;
    // Assuming we want CVPixelBufferRef for now
    return frame->data[3];
}

static void virtual_interface_get_frame_ref_yuv_data(void* frame_ref, uint8_t*** data,
                                                     int** linesize, int* width, int* height,
                                                     int* visible_width, int* visible_height) {
    AVFrame* frame = (AVFrame*)frame_ref;
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
    AVFrame* frame = (AVFrame*)frame_ref;
    av_frame_free(&frame);
}

void virtual_interface_send_frame(AVFrame* frame) {
    if (!connected) return;
    std::lock_guard<std::mutex> guard(whist_window_mutex);

    // Update the pending frame
    av_frame_free(&pending);
    pending = av_frame_clone(frame);

    // For each window,
    for (auto const& [window_id, window_info] : whist_windows) {
        // If that window wants a callback, give it
        if (window_info.video_frame_callback_ptr) {
            AVFrame* frame_ref = av_frame_clone(frame);
            window_info.video_frame_callback_ptr(window_id, frame_ref);
        }
    }
}

static void virtual_interface_set_video_frame_callback(int window_id,
                                                       VideoFrameCallback callback_ptr) {
    std::lock_guard<std::mutex> guard(whist_window_mutex);
    // If there's a pending avframe, and that window hasn't been capturing yet,
    // pass the existant frame into the callback ptr
    if (pending != NULL && whist_windows[window_id].video_frame_callback_ptr == NULL) {
        AVFrame* frame_ref = av_frame_clone(pending);
        callback_ptr(window_id, frame_ref);
    }
    // Store the callback ptr for future frames
    whist_windows[window_id].video_frame_callback_ptr = callback_ptr;
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

static int virtual_interface_create_window(void) {
    std::lock_guard<std::mutex> guard(whist_window_mutex);
    // Use serial window IDs, so that each window gets a unique ID
    static int serial_window_ids = 1;
    int next_window_id = serial_window_ids++;
    // Store window info, and return the new window ID
    whist_windows[next_window_id] = {};
    return next_window_id;
}

static void virtual_interface_destroy_window(int window_id) {
    std::lock_guard<std::mutex> guard(whist_window_mutex);
    whist_windows.erase(window_id);
}

static void virtual_interface_set_callback_context(void* context) { callback_context = context; }

static const VirtualInterface vi = {
    .lifecycle =
        {
            .connect = virtual_interface_connect,
            .create_window = virtual_interface_create_window,
            .destroy_window = virtual_interface_destroy_window,
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
