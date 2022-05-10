#include "common.h"
#include "interface.h"

static WhistMutex lock;
static AVFrame* pending = NULL;
static bool connected = false;

void virtual_interface_connect(void) {
    lock = whist_create_mutex();
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

void virtual_interface_get_frame_ref_nv12_data(void* frame_ref, uint8_t*** data, int** linesize,
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
}
