#ifndef INTERFACE_H
#define INTERFACE_H

void virtual_interface_send_frame(AVFrame* frame);

// External only
void virtual_interface_connect(void);
void* virtual_interface_get_frame_ref(void);
void* virtual_interface_get_handle_from_frame_ref(void* frame_ref);
void virtual_interface_free_frame_ref(void* frame_ref);
void virtual_interface_disconnect(void);

#endif  // INTERFACE_H
