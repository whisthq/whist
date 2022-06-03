#include "common.h"
#include "interface.h"
#include <whist/utils/queue.h>

// Just chosen a very large number for events queue size. If required we can optimize/reduce it.
#define MAX_EVENTS_QUEUED 10000

QueueContext* events_queue = NULL;

static WhistMutex lock;
static AVFrame* pending = NULL;
static bool connected = false;

void virtual_interface_connect(void) {
    lock = whist_create_mutex();
    events_queue = fifo_queue_create(sizeof(WhistFrontendEvent), MAX_EVENTS_QUEUED);
    connected = true;
}

void* virtual_interface_get_frame_ref(void) {
    whist_lock_mutex(lock);
    void* out = pending;
    pending = NULL;
    whist_unlock_mutex(lock);
    return out;
}

void* virtual_interface_get_handle_from_frame_ref(void* frame_ref) {
    AVFrame* frame = frame_ref;
    // Assuming we want CVPixelBufferRef for now
    return frame->data[3];
}

void virtual_interface_get_frame_ref_yuv_data(void* frame_ref, uint8_t*** data, int** linesize,
                                              int* width, int* height) {
    AVFrame* frame = frame_ref;
    *data = frame->data;
    *linesize = frame->linesize;
    *width = frame->width;
    *height = frame->height;
}

void virtual_interface_free_frame_ref(void* frame_ref) {
    AVFrame* frame = frame_ref;
    av_frame_free(&frame);
}

void virtual_interface_send_frame(AVFrame* frame) {
    if (!connected) return;

    whist_lock_mutex(lock);
    av_frame_free(&pending);
    pending = av_frame_clone(frame);
    whist_unlock_mutex(lock);
}

void virtual_interface_disconnect(void) {
    connected = false;
    whist_destroy_mutex(lock);
    fifo_queue_destroy(events_queue);
}

void virtual_interface_send_event(const WhistFrontendEvent* frontend_event) {
    if (fifo_queue_enqueue_item(events_queue, frontend_event) != 0) {
        LOG_ERROR("Virtual event queuing failed");
    }
}
