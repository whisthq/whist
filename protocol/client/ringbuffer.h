#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <fractal/core/fractal.h>
#include "network.h"

typedef enum FrameDataType {
	FRAME_AUDIO,
	FRAME_VIDEO,
} FrameDataType;

typedef struct FrameData {
	FrameDataType type;
	int num_packets;
	int id;
	int packets_received;
	int frame_size;
	bool* received_indices;
	bool* nacked_indices;
	char* frame_buffer;

	int num_times_nacked;
	int last_nacked_index;
	clock last_nacked_timer;
	clock last_packet_timer;
	clock frame_creation_timer;
	bool rendered;
} FrameData;

typedef struct RingBuffer {
	int ring_buffer_size;
	FrameData* receiving_frames;
	FrameDataType type;
	int largest_frame_size;

	int last_received_id;
	int num_nacked;
	int frames_received;
	int max_id;
	clock missing_frame_nack_timer;

	BlockAllocator* frame_buffer_allocator; // unusued if audio

} RingBuffer;

RingBuffer* init_ring_buffer(FrameDataType type, int ring_buffer_size);
FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id);
int receive_packet(RingBuffer* ring_buffer, FractalPacket* packet);
void destroy_ring_buffer(RingBuffer* ring_buffer);
void nack_packet(RingBuffer* ring_buffer, int id, int index);

#endif
