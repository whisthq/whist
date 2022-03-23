#pragma once

int audio_path_init(void *frontend);

int push_to_audio_path(int id, unsigned char *buf, int size);

int pop_from_audio_path(unsigned char *buf, int *size);

void push_to_audio_path_udp_cb(void *data);
