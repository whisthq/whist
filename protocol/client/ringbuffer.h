#ifndef RINGBUFFER_H
#define RINGBUFFER_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ringbuffer.h
 * @brief This file contains the code to create a ring buffer and use it for handling received
audio/video.
============================
Usage
============================
Initialize a ring buffer for audio or video using init_ring_buffer. When new packets arrive, call
receive_packet to process the packet and modify or create ring buffer entries as needed. To nack a
packet, call nack_packet.
*/

#include <fractal/core/fractal.h>
#include "network.h"

/**
 * @brief 	Audio/video types for ring buffers and frames
 */
typedef enum FrameDataType {
    FRAME_AUDIO,
    FRAME_VIDEO,
} FrameDataType;

/**
 * @brief FrameData struct containing content and metadata of encoded frames.
 * @details This is used to handle reconstruction of encoded frames from UDP packets. It contains
 * metadata to keep track of what packets we have received and nacked for, as well as a buffer for
 * holding the concatenated UDP packets.
 */
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

/**
 * @brief	RingBuffer struct for abstracting away frame reconstruction and frame retrieval.
 * @details This is used by client/audio.c and client/video.c to keep track of frames as the client
 * receives packets. It handles inserting new packets into the ring buffer and nacking for missing
 * packets.
 */
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

    BlockAllocator* frame_buffer_allocator;  // unusued if audio

} RingBuffer;

/**
 * @brief Initializes a ring buffer of the specified size and type.
 *
 * @param type Either an audio or video ring buffer.
 *
 * @param ring_buffer_size The desired size of the ring buffer
 *
 * @returns A pointer to the newly created ring buffer. All frames in the new ring buffer have ID
 * -1.
 */
RingBuffer* init_ring_buffer(FrameDataType type, int ring_buffer_size);

/**
 * @brief Retrives the frame at the given ID in the ring buffer.
 *
 * @param ring_buffer Ring buffer to access
 *
 * @param id ID of desired frame.
 *
 * @returns Pointer to the FrameData at ID id in ring_buffer.
 */
FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id);

/**
 * @brief Add a packet to the ring buffer, and initialize the corresponding frame if necessary. Also
 * nacks for missing packets or frames.
 *
 * @param ring_buffer Ring buffer to place packet into
 *
 * @param packet An audio or video UDP packet
 *
 * @returns 0 on success, -1 on failure
 */
int receive_packet(RingBuffer* ring_buffer, FractalPacket* packet);

/**
 * @brief Free the ring buffer and all its constituent frames.
 *
 * @param ring_buffer Ring buffer to destroy
 */
void destroy_ring_buffer(RingBuffer* ring_buffer);

/**
 * @brief Send a nack for the packet at index index of frame with ID id.
 *
 * @param ring_buffer Ring buffer that should have the packet
 *
 * @param id ID of the frame that should have the packet
 *
 * @param index Packet index
 */
void nack_packet(RingBuffer* ring_buffer, int id, int index);

#endif
