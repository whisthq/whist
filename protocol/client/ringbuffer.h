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
#include <fractal/core/fractal_frame.h>

#define MAX_PACKET_NACKS 2

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
    int* nacked_indices;
    char* frame_buffer;

    int num_times_nacked;
    int last_nacked_index;
    clock last_nacked_timer;
    clock last_packet_timer;
    clock frame_creation_timer;
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
    int largest_num_packets;

    int currently_rendering_id;
    FrameData currently_rendering_frame;
    // The ID of the last received normal packet (Ignoring nacks)
    int last_received_nonnack_id;
    int num_nacked;
    int frames_received;
    int max_id;
    clock missing_frame_nack_timer;

    BlockAllocator* frame_buffer_allocator;  // unused if audio
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

/**
 * @brief Nack 1 missing packet of frame_data up to index.
 *
 * @param ring_buffer Ring buffer containing the frame
 *
 * @param frame_data Frame that might be missing packets
 *
 * @param index Packet index to nack up to
 */
void nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int index);

/**
 * @brief Reset the frame's metadata.
 *
 * @param frame_data Frame to "clear" from the ring buffer.
 */
void reset_frame(FrameData* frame_data);

/**
 * @brief       Indicate that the frame with ID id is currently rendering, and free the frame buffer
 *              for the previously rendered frame. The ring buffer will not write to the currently
 *              rendering frame.
 *
 * @param ring_buffer Ring buffer containing the frame
 *
 * @param id ID of the frame we are currently rendering.
 */
void set_rendering(RingBuffer* ring_buffer, int id);

#endif
