#ifndef INTERFACE_H
#define INTERFACE_H

#include "../frontend.h"

void virtual_interface_send_frame(AVFrame* frame);

// External only
void virtual_interface_connect(void);
void* virtual_interface_get_frame_ref(void);
void* virtual_interface_get_handle_from_frame_ref(void* frame_ref);
void virtual_interface_get_frame_ref_nv12_data(void* frame_ref, uint8_t*** data, int** linesize,
                                               int* width, int* height);
void virtual_interface_free_frame_ref(void* frame_ref);
void virtual_interface_disconnect(void);

void virtual_interface_send_event(const WhistFrontendEvent* frontend_event);

#endif  // INTERFACE_H
