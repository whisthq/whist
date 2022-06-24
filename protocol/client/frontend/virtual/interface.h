#ifndef INTERFACE_H
#define INTERFACE_H

#include "../frontend.h"

typedef const char* (*FileUploadCallback)(void*);

void virtual_interface_send_frame(AVFrame* frame);
const char* virtual_interface_on_file_upload(void);

// External only
void virtual_interface_connect(void);
void* virtual_interface_get_frame_ref(void);
void* virtual_interface_get_handle_from_frame_ref(void* frame_ref);
void virtual_interface_get_frame_ref_yuv_data(void* frame_ref, uint8_t*** data, int** linesize,
                                              int* width, int* height, int* visible_width,
                                              int* visible_height);
void virtual_interface_free_frame_ref(void* frame_ref);
void virtual_interface_disconnect(void);

void virtual_interface_send_event(const WhistFrontendEvent* frontend_event);

void virtual_interface_set_fileupload_callback(FileUploadCallback callback, void* arg);

#endif  // INTERFACE_H
