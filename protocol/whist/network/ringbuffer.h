#ifndef RINGBUFFER_H
#define RINGBUFFER_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file ringbuffer.h
 * @brief This file contains the code to create a ring buffer and use it for handling received
audio/video.
============================
Usage
============================
Initialize a ring buffer for audio or video using init_ring_buffer. When new packets arrive, call
receive_packet to process the packet and modify or create ring buffer entries as needed. To nack a
packet, call nack_single_packet.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include <whist/utils/fec.h>
#include <whist/network/network_algorithm.h>

/*
============================
Defines
============================
*/

/**
 * @brief FrameData struct containing content and metadata of encoded frames.
 * @details This is used to handle reconstruction of encoded frames from UDP packets. It contains
 * metadata to keep track of what packets we have received and nacked for, as well as a buffer for
 * holding the concatenated UDP packets.
 *
 * TODO: Pull this into RingBuffer.c, and stop exposing it to the client
 */
typedef struct FrameData {
    WhistPacketType type;
    int num_original_packets;
    int num_fec_packets;
    int id;
    int original_packets_received;
    int fec_packets_received;
    bool* received_indices;
    char* packet_buffer;

    // When the FrameData is being rendered,
    // this is the data that's being rendered
    char* frame_buffer;
    int frame_buffer_size;

    // FEC logic
    char* fec_frame_buffer;
    bool successful_fec_recovery;

    // Nack logic

    // Whether or not we're in "recovery mode"
    bool recovery_mode;
    int* num_times_index_nacked;
    int num_times_nacked;
    int last_nacked_index;
    WhistTimer last_nacked_timer;
    WhistTimer last_nonnack_packet_timer;
    WhistTimer frame_creation_timer;
    FECDecoder* fec_decoder;
} FrameData;

// Handler that gets called when the ring buffer wants to nack for a packet
typedef void (*NackPacketFn)(SocketContext* socket_context, WhistPacketType frame_type, int id,
                             int index);
typedef void (*StreamResetFn)(SocketContext* socket_context, WhistPacketType frame_type,
                              int last_failed_id);

/**
 * @brief	RingBuffer struct for abstracting away frame reconstruction and frame retrieval.
 * @details This is used by client/audio.c and client/video.c to keep track of frames as the client
 * receives packets. It handles inserting new packets into the ring buffer and nacking for missing
 * packets.
 */
typedef struct RingBuffer {
    int ring_buffer_size;
    FrameData* receiving_frames;
    WhistPacketType type;
    int largest_frame_size;

    // networking interface
    SocketContext* socket_context;
    NackPacketFn nack_packet;
    StreamResetFn request_stream_reset;

    BlockAllocator* packet_buffer_allocator;  // unused if audio

    int currently_rendering_id;
    FrameData currently_rendering_frame;

    // *** START OF BITRATE STAT CALCULATIONS ***
    WhistTimer network_statistics_timer;
    int num_packets_nacked;
    int num_packets_received;
    int num_frames_rendered;
    // *** END OF BITRATE STAT CALCULATIONS ***

    int frames_received;
    int max_id;

    // Nack variables

    // The next ID that should be rendered, marks
    // the lowest packet ID we're interested in nacking about
    int last_rendered_id;
    int last_missing_frame_nack;
    int most_recent_reset_id;
    WhistTimer last_stream_reset_request_timer;
    int last_stream_reset_request_id;

    // Nacking bandwidth tracker
    WhistTimer burst_timer;
    WhistTimer avg_timer;
    int burst_counter;
    int avg_counter;
    bool last_nack_possibility;
} RingBuffer;

/*
============================
Public Functions
============================
*/

/**
 * @brief                           Initializes a ring buffer of the specified size and type.
 *
 * @param type                      Either an audio or video ring buffer.
 *
 * @param max_frame_size            The largest a frame can get
 *
 * @param ring_buffer_size          The desired size of the ring buffer
 *
 * @param socket_context            A temporary parameter to call the lambda functions
 *                                  without std::bind
 *                                  TODO: Remove
 *
 * @param nack_packet               A lambda function that will be called when the ring buffer
 *                                  wants to nack for something. NULL will disable nacking.
 *
 * @param request_stream_reset      A temporary lambda function to make the refactor work
 *                                  NULL to disable
 *                                  TODO: Remove
 *
 * @returns                         A pointer to the newly created and now empty ring buffer
 */
RingBuffer* init_ring_buffer(WhistPacketType type, int max_frame_size, int ring_buffer_size,
                             SocketContext* socket_context, NackPacketFn nack_packet,
                             StreamResetFn request_stream_reset);

/**
 * @brief Add a packet to the ring buffer, and initialize the corresponding frame if necessary. Also
 * nacks for missing packets or frames.
 *
 * @param ring_buffer Ring buffer to place packet into
 *
 * @param segment The segment of an audio or video WhistPacket
 *
 * @returns 0 on success, -1 on failure
 *
 * TODO: Remove returning 1 when overwriting a valid frame
 * (int): 1 if we overwrote a valid frame, 0 on success, -1 on failure
 */
int ring_buffer_receive_segment(RingBuffer* ring_buffer, WhistSegment* segment);

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
 * @brief       Indicate that the frame with ID id is currently rendering, and free the frame buffer
 *              for the previously rendered frame. The ring buffer will not write to the currently
 *              rendering frame.
 *
 * @param ring_buffer Ring buffer containing the frame
 *
 * @param id ID of the frame we want to render
 *
 * @returns true if the frame is ready, false if it's not
 */
bool is_ready_to_render(RingBuffer* ring_buffer, int id);

/**
 * @brief       Indicate that the frame with ID id is currently rendering, and free the frame buffer
 *              for the previously rendered frame. The ring buffer will not write to the currently
 *              rendering frame.
 *
 * @param ring_buffer Ring buffer containing the frame
 *
 * @param id ID of the frame we are currently rendering.
 *
 * @returns      A new pointer to the frame buffer that's now being rendered.
 *               This pointer will be invalidated on the next call to set_rendering.
 *               Use ->frame_buffer to read the contents of the captured frame
 */
FrameData* set_rendering(RingBuffer* ring_buffer, int id);

/**
 * @brief                          Skip the ring buffer to ID id,
 *                                 dropping all packets prior to that id
 *
 * @param ring_buffer              Ring buffer to use
 * @param id                       The ID to skip to
 */
void reset_stream(RingBuffer* ring_buffer, int id);

/**
 * @brief                          Try nacking or requesting a stream reset
 *
 * @param ring_buffer              Ring buffer to use
 * @param latency                  The latency, used in figuring out when to nack / stream reset
 *
 * @note                           This will call the lambda nack_packet and request_stream_reset
 *                                 TODO: Just make this return an array of nacked packets,
 *                                       or a stream reset request with last failed ID
 */
void try_recovering_missing_packets_or_frames(RingBuffer* ring_buffer, double latency);

/**
 * @brief                         Get network statistics from the ringbuffer
 *
 * @param ring_buffer             The ringbuffer to get network statistics from
 *
 * @note                          Statistics will be taken from the time between the current time,
 *                                and the last call to get_network_statistics
 *                                (Or, from init_ring_buffer, on the first call)
 */
NetworkStatistics get_network_statistics(RingBuffer* ring_buffer);

/**
 * @brief Destroy the ringbuffer and all associated memory
 *
 * @param ring_buffer Ring buffer to destroy
 */
void destroy_ring_buffer(RingBuffer* ring_buffer);

#endif
