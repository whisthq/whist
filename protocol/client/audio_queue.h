#pragma once

int audio_queue_init(void);

int push_to_audio_queue(int id, unsigned char *buf, int size);

int pop_from_audio_queue( unsigned char *buf, int *size);


