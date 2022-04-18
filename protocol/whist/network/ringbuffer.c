/*
============================
Includes
============================
*/

#include "ringbuffer.h"
#include <assert.h>
#include <whist/fec/fec.h>
#include <whist/logging/log_statistic.h>
#include <whist/debug/protocol_analyzer.h>
#include <whist/debug/debug_console.h>
#include "whist/utils/string_buffer.h"

/*
============================
Defines
============================
*/

#define MAX_RING_BUFFER_SIZE 500
#define MAX_UNORDERED_PACKETS 10

// If a frame is totally missing, this is how many packets we'll NACK for
// in that missing frame.
// Frames of size <= NACK_PACKETS_MISSING_FRAME, will be able to recover in a single RTT
#define NACK_PACKETS_MISSING_FRAME 20

// Estimated ratio of network jitter to latency
// TODO: Use actual recorded jitter here
#define ESTIMATED_JITTER_LATENCY_RATIO 0.3

#define NACK_STATISTICS_SEC 5.0
// The max number of times we can NACK for a packet
#define MAX_PACKET_NACKS 2

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Initialize the frame with the given parameters
 *
 * @param ring_buffer              Ring buffer containing the frame
 * @param id                       The ID of the frame to initialize
 * @param num_original_indices     The number of original indices
 * @param num_fec_indices          The number of FEC indices
 * @param prev_frame_num_duplicates The number of duplicate filler packets that were sent for
 *                                  previous frame
 */
void init_frame(RingBuffer* ring_buffer, int id, int num_original_indices, int num_fec_indices,
                int prev_frame_num_duplicates);

/**
 * @brief                          Reset the given frame, freeing all of its data,
 *                                 and marking it as freed
 *
 * @param ring_buffer              Ring buffer containing the frame
 * @param frame_data               Frame to reset from the ring buffer
 */
static void reset_frame(RingBuffer* ring_buffer, FrameData* frame_data);

/**
 * @brief                         Reset the ringbuffer to be identical to a
 *                                newly initialized ringbuffer, with all frames freed
 *
 * @param ring_buffer             Ring buffer to reset
 */
static void reset_ring_buffer(RingBuffer* ring_buffer);

/**
 * @brief                         Nack all of the missing packets up to end_index
 *
 * @param ring_buffer             Ring buffer to nack with
 * @param id                      The ID of the packet we're nacking for
 * @param index                   The index of the UDPPacket we're nacking for
 */
static void nack_single_packet(RingBuffer* ring_buffer, int id, int index);

/**
 * @brief                         Nack all of the missing packets up to end_index
 *
 * @param ring_buffer             Ring buffer to nack with
 * @param frame_data              The frame whose indices we will be nacking
 * @param end_index               The last index that's considered "missing"
 * @param max_packets_to_nack     The maximum number of packets that we can nack for
 *
 * @returns                       The number of packets that have been nacked
 */
static int nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data,
                                            int end_index, int max_packets_to_nack, double latency);

/**
 * @brief If any packets are still missing, and it's been too long, try nacking for them.
 *        Ideally, this gets called quite rapidly, it has internal timers to throttle nacks.
 *        The more rapidly the better, just need to balance CPU usage, 5-10ms should be fine.
 *
 * @param ring_buffer The ring buffer to try nacking with
 *
 * @param latency The round-trip latency of the connection. Helpful with nacking logic
 *
 * @param max_unordered_packets Maximum number of packets that can arrive out-of-order
 *
 * @param network_settings Pointer to NetworkSettings structure for accessing the bitrate and
 *                         burst_bitrate
 *
 * @returns     True if nacking succeded,
 *              False if we've bandwidth saturated our ability to nack.
 */
static bool try_nacking(RingBuffer* ring_buffer, double latency, int max_unordered_packets,
                        NetworkSettings* network_settings);

// TODO: document this
char* get_framebuffer(RingBuffer* ring_buffer, FrameData* current_frame);

static double latency_plus_jitter(double latency) {
    // In addition to network latency and jitter, throttler could also add a latency of
    // UDP_NETWORK_THROTTLER_BUCKET_MS
    return ((1.0 + ESTIMATED_JITTER_LATENCY_RATIO) * latency) +
           (UDP_NETWORK_THROTTLER_BUCKET_MS / MS_IN_SECOND);
}

/*
============================
Public Function Implementations
============================
*/

RingBuffer* init_ring_buffer(WhistPacketType type, int max_frame_size, int ring_buffer_size,
                             SocketContext* socket_context, NackPacketFn nack_packet,
                             StreamResetFn request_stream_reset) {
    /*
        Initialize the ring buffer; malloc space for all the frames and set their IDs to -1.

        Arguments:
            type (WhistPacketType): audio or video
            ring_buffer_size (int): Desired size of the ring buffer. If the size exceeds
       MAX_RING_BUFFER_SIZE, no ring buffer is returned.

        Returns:
            (RingBuffer*): pointer to the created ring buffer
    */
    FATAL_ASSERT(ring_buffer_size <= MAX_RING_BUFFER_SIZE);

    RingBuffer* ring_buffer = safe_malloc(sizeof(RingBuffer));

    ring_buffer->type = type;
    ring_buffer->ring_buffer_size = ring_buffer_size;
    ring_buffer->receiving_frames = safe_malloc(ring_buffer_size * sizeof(FrameData));
    ring_buffer->socket_context = socket_context;
    ring_buffer->nack_packet = nack_packet;
    ring_buffer->request_stream_reset = request_stream_reset;

    // Mark all the frames as uninitialized
    for (int i = 0; i < ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        frame_data->id = frame_data->entire_frame_nacked_id = -1;
        frame_data->packet_buffer = NULL;
    }

    // determine largest frame size, including the WhistPacket header
    ring_buffer->largest_frame_size = sizeof(WhistPacket) - MAX_PAYLOAD_SIZE + max_frame_size;

    ring_buffer->packet_buffer_allocator = create_block_allocator(ring_buffer->largest_frame_size);
    ring_buffer->currently_rendering_id = -1;
    ring_buffer->last_rendered_id = -1;

    // set all additional metadata for frames and ring buffer
    reset_ring_buffer(ring_buffer);
    // reset bitrate stat variables
    ring_buffer->num_packets_nacked = 0;
    ring_buffer->num_packets_received = 0;
    ring_buffer->num_frames_rendered = 0;

    start_timer(&ring_buffer->network_statistics_timer);

    // Nack bandwidth tracking
    ring_buffer->burst_counter = 0;
    start_timer(&ring_buffer->burst_timer);
    ring_buffer->avg_counter = 0;
    start_timer(&ring_buffer->avg_timer);
    ring_buffer->last_nack_possibility = true;

    start_timer(&ring_buffer->last_stream_reset_request_timer);
    ring_buffer->last_stream_reset_request_id = -1;

    // Nack statistics tracking
    ring_buffer->num_nacks_received = 0;
    ring_buffer->num_original_packets_received = 0;
    ring_buffer->num_unnecessary_original_packets_received = 0;
    ring_buffer->num_unnecessary_nacks_received = 0;
    ring_buffer->num_times_nacking_saturated = 0;
    start_timer(&ring_buffer->last_nack_statistics_timer);

    return ring_buffer;
}

bool ring_buffer_receive_segment(RingBuffer* ring_buffer, WhistSegment* segment) {
    whist_analyzer_record_segment(segment);
    // Sanity check the packet's metadata
    WhistPacketType type = segment->whist_type;
    int segment_id = segment->id;
    unsigned short segment_index = segment->index;
    unsigned short num_indices = segment->num_indices;
    unsigned short num_fec_indices = segment->num_fec_indices;
    unsigned short segment_size = segment->segment_size;
    FATAL_ASSERT(segment_index < num_indices);
    FATAL_ASSERT(num_indices <= MAX_PACKETS);
    FATAL_ASSERT(num_fec_indices < num_indices);
    FATAL_ASSERT(segment_size <= MAX_PACKET_SEGMENT_SIZE);

    FrameData* frame_data = get_frame_at_id(ring_buffer, segment_id);

    ring_buffer->num_packets_received++;

    // Whether or not the ringbuffer overflowed, which controls the return value
    bool ringbuffer_overflowed = false;

    // If segment_id != frame_data->id, handle the situation
    if (segment_id < frame_data->id) {
        // This packet must be from a very stale frame,
        // because the current ringbuffer occupant already contains packets with a newer ID in it
        LOG_WARNING("Very stale packet (ID %d) received, current ringbuffer occupant's ID %d",
                    segment_id, frame_data->id);
        ring_buffer->num_unnecessary_original_packets_received++;
        return !ringbuffer_overflowed;
    } else if (segment_id > frame_data->id) {
        // This packet is newer than the resident,
        // so it's time to overwrite the resident if such a resident exists
        if (frame_data->packet_buffer != NULL) {
            if (frame_data->id > ring_buffer->currently_rendering_id) {
                whist_analyzer_record_reset_ringbuffer(
                    ring_buffer->type, ring_buffer->currently_rendering_id, segment_id);
                // We have received a packet, which will overwrite a frame that needs to be rendered
                // in the future. This means that stream-reset failed to recover within the
                // size of the ringbuffer, and this failure will be logged accordingly.
                LOG_WARNING(
                    "We received a %s packet with Frame ID %d, that is trying to overwrite Frame "
                    "ID %d.\n"
                    "But we can't overwrite that frame, since our renderer has only rendered up to "
                    "ID %d.\n"
                    "Resetting the entire ringbuffer...",
                    type == PACKET_VIDEO ? "video" : "audio", segment_id, frame_data->id,
                    ring_buffer->currently_rendering_id);
                // Log out the contents of the entire ringbuffer
                for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
                    FrameData* dropped_frame_data = get_frame_at_id(ring_buffer, i);
                    if (dropped_frame_data->id != -1) {
                        // Get whether or not the stream could have recovered at that point
                        bool is_recovery_point = false;
                        if (is_ready_to_render(ring_buffer, dropped_frame_data->id)) {
                            if (type == PACKET_VIDEO) {
                                // Grab whether or not the video is a recovery point
                                WhistPacket* whist_packet =
                                    (WhistPacket*)dropped_frame_data->frame_buffer;
                                VideoFrame* video_frame = (VideoFrame*)whist_packet->data;
                                if (VIDEO_FRAME_TYPE_IS_RECOVERY_POINT(video_frame->frame_type)) {
                                    is_recovery_point = true;
                                }
                            } else {
                                // All audio frames are valid recovery points
                                is_recovery_point = true;
                            }
                        }
                        // Log the dropped frame, along with missing indices and NACKing info on
                        // those indices
                        LOG_WARNING("Frame[%d] dropped with ID %d: %d/%d %s", i,
                                    dropped_frame_data->id,
                                    dropped_frame_data->original_packets_received,
                                    dropped_frame_data->num_original_packets,
                                    is_recovery_point ? "(Recovery Frame)" : "");
                        for (int j = 0; j < dropped_frame_data->num_original_packets; j++) {
                            if (!dropped_frame_data->received_indices[j]) {
                                LOG_WARNING("Did not receive ID %d, Index %d. Nacked %d times.", i,
                                            j, dropped_frame_data->num_times_index_nacked[j]);
                            }
                        }
                    } else {
                        LOG_WARNING("Frame[%d] is empty", i);
                    }
                }
                // Wipe the ringbuffer, and mark as overflowed
                reset_ring_buffer(ring_buffer);
                ringbuffer_overflowed = true;
            } else {
                // Here, the frame is older than where our renderer is,
                // So we can just reset the undesired frame
                LOG_ERROR(
                    "Trying to allocate %s Frame ID %d, but Frame ID %d has not been destroyed "
                    "yet! Destroying it now...",
                    type == PACKET_VIDEO ? "video" : "audio", segment_id, frame_data->id);
                reset_frame(ring_buffer, frame_data);
            }
        }

        // Initialize the frame now, so that it can hold the packet we just received
        int num_original_packets = num_indices - num_fec_indices;
        init_frame(ring_buffer, segment_id, num_original_packets, num_fec_indices,
                   segment->prev_frame_num_duplicates);

        // Update the ringbuffer's min/max id, with this new frame's ID
        ring_buffer->max_id = max(ring_buffer->max_id, frame_data->id);
        if (ring_buffer->min_id == -1) {
            // Initialize min_id
            ring_buffer->min_id = frame_data->id;
        } else {
            // Update min_id
            ring_buffer->min_id = min(ring_buffer->min_id, frame_data->id);
        }
    }

    // Now, the frame_data should be ready to accept the packet
    FATAL_ASSERT(segment_id == frame_data->id);

    // Verify that the packet metadata matches frame_data metadata
    FATAL_ASSERT(frame_data->num_fec_packets == num_fec_indices);
    FATAL_ASSERT(frame_data->num_original_packets + frame_data->num_fec_packets == num_indices);

    // LOG the the nacking situation
    if (segment->is_a_nack) {
        frame_data->nack_packets_received++;
        ring_buffer->num_nacks_received++;
        // Server simulates a nack for audio all the time. Hence log only for video.
        if (type == PACKET_VIDEO) {
            if (!frame_data->received_indices[segment_index]) {
                if (LOG_NACKING) {
                    LOG_INFO("NACK for video ID %d, Index %d received!", segment_id, segment_index);
                }
            } else {
                frame_data->unnecessary_nack_packets_received++;
                ring_buffer->num_unnecessary_nacks_received++;
                if (LOG_NACKING) {
                    LOG_INFO("NACK for video ID %d, Index %d received, but didn't need it.",
                             segment_id, segment_index);
                }
            }
        }
    } else {
        ring_buffer->num_original_packets_received++;
        // Reset timer since the last time we received a non-nack packet
        start_timer(&frame_data->last_nonnack_packet_timer);
        if (frame_data->num_times_index_nacked[segment_index] > 0) {
            ring_buffer->num_unnecessary_original_packets_received++;
            if (LOG_NACKING) {
                LOG_INFO("Received original %s ID %d, Index %d, but we had NACK'ed for it.",
                         type == PACKET_VIDEO ? "video" : "audio", segment_id, segment_index);
            }
        }
    }

    // If we have already received this packet anyway, just drop this packet
    if (frame_data->received_indices[segment_index]) {
        if (!segment->is_a_nack) {
            frame_data->duplicate_packets_received++;
        }
        // The only way it should possible to receive a packet twice, is if nacking got involved
        if (type == PACKET_VIDEO && frame_data->num_times_index_nacked[segment_index] == 0 &&
            !segment->is_a_duplicate) {
            LOG_ERROR(
                "We received a video packet (ID %d / index %d) twice, but we had never nacked for "
                "it?",
                segment_id, segment_index);
            return !ringbuffer_overflowed;
        }
        return !ringbuffer_overflowed;
    }

    // Remember whether or not this frame was ready to render
    bool was_already_ready = is_ready_to_render(ring_buffer, segment_id);

    // Track whether the index we received is one of the N original packets,
    // or one of the M FEC packets
    frame_data->received_indices[segment_index] = true;
    if (segment_index < frame_data->num_original_packets) {
        frame_data->original_packets_received++;
        FATAL_ASSERT(frame_data->original_packets_received <= frame_data->num_original_packets);
    } else {
        frame_data->fec_packets_received++;
    }

    if (segment_id <= ring_buffer->currently_rendering_id) {
        // This packet won't help us render any new packets,
        // So don't proceed further
        ring_buffer->num_unnecessary_original_packets_received++;
        return !ringbuffer_overflowed;
    }

    // Copy the packet's payload into the correct location in frame_data's frame buffer
    int buffer_offset = segment_index * MAX_PACKET_SEGMENT_SIZE;
    if (buffer_offset + segment_size >= ring_buffer->largest_frame_size) {
        LOG_ERROR("Packet payload too large for frame buffer! Dropping the packet...");
        return !ringbuffer_overflowed;
    }
    FATAL_ASSERT(frame_data->packet_buffer != NULL);
    memcpy(frame_data->packet_buffer + buffer_offset, segment->segment_data, segment_size);

    // If this frame isn't an fec frame, the frame_buffer_size is just the sum of the payload sizes
    if (frame_data->num_fec_packets == 0) {
        frame_data->frame_buffer_size += segment_size;
    }

    // If this is an FEC frame, and we haven't yet decoded the frame successfully,
    // Try decoding the FEC frame
    if (frame_data->num_fec_packets > 0 && !frame_data->successful_fec_recovery) {
        // Register this packet into the FEC decoder
        fec_decoder_register_buffer(frame_data->fec_decoder, segment_index,
                                    frame_data->packet_buffer + buffer_offset, segment_size);

        WhistTimer decode_timer;
        start_timer(&decode_timer);
        // Using the newly registered packet, try to decode the frame using FEC
        int frame_size =
            fec_get_decoded_buffer(frame_data->fec_decoder, frame_data->fec_frame_buffer);
        double decode_time = get_timer(&decode_timer) * MS_IN_SECOND;

        // If we were able to successfully decode the frame, mark it as such!
        if (frame_size >= 0) {
            if (frame_data->original_packets_received < frame_data->num_original_packets) {
                if (LOG_FEC_DECODE) {
                    LOG_INFO(
                        "[FEC] Successfully recovered %d/%d Packet %d, using %d FEC packets, in "
                        "%fms",
                        frame_data->original_packets_received, frame_data->num_original_packets,
                        frame_data->id, frame_data->fec_packets_received, decode_time);
                }
                whist_analyzer_record_fec_used(type, segment_id);
            }
            // Save the frame buffer size of the fec frame,
            // And mark the fec recovery as succeeded
            frame_data->frame_buffer_size = frame_size;
            frame_data->successful_fec_recovery = true;
        }
    }

    if (is_ready_to_render(ring_buffer, segment_id) && !was_already_ready) {
        ring_buffer->frames_received++;
    }

    return !ringbuffer_overflowed;
}

FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id) {
    return &ring_buffer->receiving_frames[id % ring_buffer->ring_buffer_size];
}

double get_packet_loss_ratio(RingBuffer* ring_buffer, double latency) {
    int num_packets_received = 0;
    int num_packets_sent = 0;
    // Don't include max_id frame for computing packet loss as it could be in progress
    for (int id = max(max(ring_buffer->max_id - MAX_FPS, ring_buffer->min_id), 0);
         id < ring_buffer->max_id; id++) {
        FrameData* frame = get_frame_at_id(ring_buffer, id);
        if (id != frame->id ||
            get_timer(&frame->frame_creation_timer) > (double)PACKET_LOSS_DURATION_IN_SEC) {
            continue;
        }
        num_packets_sent += frame->num_original_packets;
        num_packets_sent += frame->num_fec_packets;
        num_packets_received += frame->original_packets_received;
        num_packets_received += frame->fec_packets_received;

        int nacks_sent = 0;
        for (int i = 0; i < frame->num_original_packets + frame->num_fec_packets; i++) {
            if (frame->num_times_index_nacked[i]) {
                nacks_sent += frame->num_times_index_nacked[i];
                // Don't consider the NACKs in flight
                if (get_timer(&frame->last_nacked_timer[i]) < latency_plus_jitter(latency)) {
                    nacks_sent--;
                }
            }
        }
        // Whatever we considered as "NACKs in flight" could have arrived earlier. So if the actual
        // nack_packets_received is greater our estimate of NACK requests sent, then just
        // approximate nack_requests_sent = nack_packets_received.
        nacks_sent = max(nacks_sent, frame->nack_packets_received);

        // Currently we don't have any way of knowing in the NACK request packet from client to
        // server got lost OR if the NACK response packet from server to client got lost. As a
        // reasonable approximation we are considering both cases as NACK packet loss from server to
        // client
        num_packets_sent += nacks_sent;
        num_packets_received += frame->unnecessary_nack_packets_received;

        FrameData* next_frame = get_frame_at_id(ring_buffer, id + 1);
        if (id + 1 == next_frame->id) {
            num_packets_sent += next_frame->prev_frame_num_duplicate_packets;
            num_packets_received += frame->duplicate_packets_received;
        }
    }
    double packet_loss_ratio = 0.0;
    if (num_packets_sent > 0 && num_packets_received > 0) {
        packet_loss_ratio =
            (double)(num_packets_sent - num_packets_received) / (double)num_packets_sent;
    }
    return packet_loss_ratio;
}

bool is_ready_to_render(RingBuffer* ring_buffer, int id) {
    FrameData* current_frame = get_frame_at_id(ring_buffer, id);
    // A frame ID ready to render, if the ID exists in the ringbuffer,
    if (current_frame->id != id) {
        return false;
    }
    // Set the frame buffer so that others can read from it
    current_frame->frame_buffer = get_framebuffer(ring_buffer, current_frame);
    // and if getting a framebuffer out of it is possible

    whist_analyzer_record_ready_to_render(ring_buffer->type, id, current_frame->frame_buffer);

    return current_frame->frame_buffer != NULL;
}

FrameData* set_rendering(RingBuffer* ring_buffer, int id) {
    /*
        Indicate that the frame with ID id is currently rendering and free the frame buffer for the
        previously rendering frame. Ownership of the frame buffer for the rendering frame is
        transferred to ring_buffer->currently_rendering_frame, allowing us to fully wipe the ring
        buffer's receiving_frames array if we fall too behind.

        Arguments:
            ring_buffer (RingBuffer*): ring buffer metadata to change
            id (int): ID of the frame we are currently rendering.
    */

    if (id <= ring_buffer->last_rendered_id) {
        LOG_FATAL("Tried to call set_rendering on an ID %d <= the last rendered ID %d", id,
                  ring_buffer->last_rendered_id);
    }

    // Set first, so that last_rendered_id is updated
    ring_buffer->last_rendered_id = id;

    if (ring_buffer->currently_rendering_id != -1) {
        // Reset the now unwanted currently rendering frame
        reset_frame(ring_buffer, &ring_buffer->currently_rendering_frame);
    }

    // Move Frame ID "id", from the ring buffer and into currently_rendering_frame
    FATAL_ASSERT(is_ready_to_render(ring_buffer, id));

    whist_analyzer_record_current_rendering(ring_buffer->type, id,
                                            ring_buffer->currently_rendering_id);

    // Move frame from current_frame, to currently_rendering_frame
    FrameData* current_frame = get_frame_at_id(ring_buffer, id);
    ring_buffer->currently_rendering_id = id;
    ring_buffer->currently_rendering_frame = *current_frame;

    // Invalidate the current_frame, without deallocating its data with reset_frame,
    // Since currently_rendering_frame now owns that data
    // Other than the dynamically allocated resources, don't clear other member variables, as it
    // will be used for packet loss calculation.
    current_frame->packet_buffer = current_frame->fec_frame_buffer = NULL;
    current_frame->fec_decoder = NULL;

    // Set the framebuffer pointer of the currently rendering frame
    ring_buffer->currently_rendering_frame.frame_buffer =
        get_framebuffer(ring_buffer, &ring_buffer->currently_rendering_frame);

    // Track for statistics
    ring_buffer->num_frames_rendered++;

    // Return the currently rendering frame
    return &ring_buffer->currently_rendering_frame;
}

// This is in the hotpath! Ensure that this function has well-bounded loops.
void reset_stream(RingBuffer* ring_buffer, int id) {
    /*
        "Skip" to the frame at ID id by setting last_rendered_id = id - 1 (if the skip is valid).
    */

    FATAL_ASSERT(1 <= id);

    // Check that we're not trying to reset to a frame that's been rendered already
    if (id <= ring_buffer->last_rendered_id) {
        LOG_INFO(
            "Received stale %s stream reset request - told to skip to ID %d but already at ID %d",
            ring_buffer->type == PACKET_VIDEO ? "video" : "audio", id,
            ring_buffer->last_rendered_id);
    } else {
        if (ring_buffer->last_rendered_id != -1) {
            // Loudly log if we're dropping frames
            LOG_INFO("Skipping from %s frame %d to frame %d",
                     ring_buffer->type == PACKET_VIDEO ? "video" : "audio",
                     ring_buffer->last_rendered_id, id);
            whist_analyzer_record_skip(ring_buffer->type, ring_buffer->last_rendered_id, id);
            // Log from the frame we were supposed to render, up until id-1
            // [id-bufsize, id-1], covers all possible moduli that could be in the ringbuffer
            for (int i = max(ring_buffer->last_rendered_id + 1, id - ring_buffer->ring_buffer_size);
                 i < id; i++) {
                FrameData* frame_data = get_frame_at_id(ring_buffer, i);
                if (frame_data->id == i) {
                    // Only verbosely log reset_stream for VIDEO frames
                    if (ring_buffer->type == PACKET_VIDEO) {
                        LOG_WARNING("Frame dropped with ID %d: %d/%d", frame_data->id,
                                    frame_data->original_packets_received,
                                    frame_data->num_original_packets);
                        for (int j = 0; j < frame_data->num_original_packets; j++) {
                            if (!frame_data->received_indices[j]) {
                                LOG_INFO("Did not receive ID %d, Index %d. Nacked %d times.", i, j,
                                         frame_data->num_times_index_nacked[j]);
                            }
                        }
                    }
                } else if (frame_data->packet_buffer != NULL) {
                    LOG_WARNING("Bad ID? %d instead of %d", frame_data->id, i);
                }
            }
        } else {
            // If we haven't rendered yet, this is just a stream-start
            LOG_INFO("Starting %s stream at frame %d",
                     ring_buffer->type == PACKET_VIDEO ? "video" : "audio", id);
        }
        // Reset all frames with a lower ID
        for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
            FrameData* frame_data = get_frame_at_id(ring_buffer, i);
            if (frame_data->packet_buffer != NULL && frame_data->id < id) {
                reset_frame(ring_buffer, frame_data);
            }
        }
        ring_buffer->last_rendered_id = id - 1;
    }
}

void try_recovering_missing_packets_or_frames(RingBuffer* ring_buffer, double latency,
                                              int max_unordered_packets,
                                              NetworkSettings* network_settings) {
    // Track NACKing statistics
    if (get_timer(&ring_buffer->last_nack_statistics_timer) > NACK_STATISTICS_SEC) {
        LOG_INFO("Current Latency: %fms", latency * MS_IN_SECOND);
        // The ratio of NACKs that actually helped recovered data
        double nack_efficiency;
        if (ring_buffer->num_nacks_received == 0) {
            LOG_INFO("No NACK packets received in last %.1f seconds", NACK_STATISTICS_SEC);
        } else {
            nack_efficiency = 1.0 - ring_buffer->num_unnecessary_nacks_received /
                                        (double)ring_buffer->num_nacks_received;
            // The ratio of the Bandwidth that consists of NACKing
            double nack_bandwidth_ratio;
            int num_total_packets_received =
                ring_buffer->num_nacks_received + ring_buffer->num_original_packets_received;
            nack_bandwidth_ratio =
                ring_buffer->num_nacks_received / (double)num_total_packets_received;
            // The percentage of NACKs that actually improved the stream
            LOG_INFO("NACK Efficiency: %.2f%%", nack_efficiency * 100);
            // The percentage of bandwidth that is NACKing
            LOG_INFO("NACK Bandwidth Ratio: %f%%", nack_bandwidth_ratio * 100);
            // The number of times we've hit the NACKing limit
            LOG_INFO("NACKing was saturated: %d times", ring_buffer->num_times_nacking_saturated);
        }
        ring_buffer->num_nacks_received = 0;
        ring_buffer->num_original_packets_received = 0;
        ring_buffer->num_unnecessary_original_packets_received = 0;
        ring_buffer->num_unnecessary_nacks_received = 0;
        ring_buffer->num_times_nacking_saturated = 0;
        start_timer(&ring_buffer->last_nack_statistics_timer);
    }

    // We should wait to receive even a single packet before trying to recover anything
    if (ring_buffer->min_id == -1) {
        return;
    }

    // Try to nack for any missing packets
    try_nacking(ring_buffer, latency, max_unordered_packets, network_settings);

    // =============
    // Stream Reset Logic
    // =============

    // Acceptable staleness = Time to transmit that frame + (RTT + Network Jitter) *
    //                        MAX_PACKET_NACKS.
    // If any Frame is "Acceptable staleness" or older,
    // Request yet another I-Frame on top of "X".
    // This recovers from the situation where we fail to receive "X".
    // The below constants are limits for acceptable staleness.
#define MIN_ACCEPTABLE_STALENESS_MS 100.0
#define MAX_ACCEPTABLE_STALENESS_MS 300.0

    // The Frame that we're currently trying our best to receive and make progress,
    // Either the next-to-render frame, or the very first frame if we haven't rendered yet.
    int currently_pending_id = ring_buffer->last_rendered_id == -1
                                   ? ring_buffer->min_id
                                   : ring_buffer->last_rendered_id + 1;

    // Track the greatest ID of any "failed" frames
    // Failed is a frame that's so far behind that we think we probably won't get it
    int greatest_failed_id = -1;

    // Only requesting a single I-Frame isn't enough, since that I-Frame could fail to receive.
    // This marks any stale frames as failed as well.
    // We only check the last 60 frames, to prevent this for-loop from taking too long
    for (int id = max(currently_pending_id, ring_buffer->max_id - 60); id <= ring_buffer->max_id;
         id++) {
        FrameData* ctx = get_frame_at_id(ring_buffer, id);

        if (ctx->id == id) {
            double frame_staleness = get_timer(&ctx->frame_creation_timer);
            double time_to_transmit = ((ctx->num_original_packets + ctx->num_fec_packets) *
                                       MAX_PAYLOAD_SIZE * BITS_IN_BYTE) /
                                      (double)network_settings->burst_bitrate;
            // Adding Network Jitter + One round-trip latency to account for nack response time
            double acceptable_staleness_ms =
                (time_to_transmit + (latency_plus_jitter(latency) * MAX_PACKET_NACKS)) *
                MS_IN_SECOND;
            acceptable_staleness_ms = max(acceptable_staleness_ms, MIN_ACCEPTABLE_STALENESS_MS);
            // If the frame is completely received, double the threshold(subject to the absolute
            // max) as there is a chance to catchup.
            if (is_ready_to_render(ring_buffer, currently_pending_id)) {
                acceptable_staleness_ms *= 2;
            }
            acceptable_staleness_ms = min(acceptable_staleness_ms, MAX_ACCEPTABLE_STALENESS_MS);

            if (frame_staleness * MS_IN_SECOND > acceptable_staleness_ms) {
                greatest_failed_id = max(greatest_failed_id, id);
            }
        }
    }

    // If we have any failed IDs, we tell the server,
    // So that it can try to give us an I-Frame
    if (greatest_failed_id != -1) {
        // Throttle the requests to prevent network upload saturation, however
        if (get_timer(&ring_buffer->last_stream_reset_request_timer) > latency / 2.0) {
            whist_analyzer_record_stream_reset(ring_buffer->type, greatest_failed_id);
            ring_buffer->request_stream_reset(ring_buffer->socket_context, ring_buffer->type,
                                              greatest_failed_id);

            // If a newer frame has failed, log it
            if (greatest_failed_id > ring_buffer->last_stream_reset_request_id) {
                FrameData* pending_ctx = get_frame_at_id(ring_buffer, currently_pending_id);
                if (pending_ctx->id != currently_pending_id) {
                    pending_ctx = NULL;
                }
                FrameData* greatest_failed_ctx = get_frame_at_id(ring_buffer, greatest_failed_id);
                if (greatest_failed_ctx->id != greatest_failed_id) {
                    greatest_failed_ctx = NULL;
                }
                LOG_INFO(
                    "The most recent ID %d is %d frames ahead of currently pending %d (%d/%d "
                    "packets received). A stream reset is now being requested to catch-up, ID's <= "
                    "%d(%d/%d packets received) are considered lost.",
                    ring_buffer->max_id, ring_buffer->max_id - currently_pending_id,
                    currently_pending_id, pending_ctx ? pending_ctx->original_packets_received : -1,
                    pending_ctx ? pending_ctx->num_original_packets : -1, greatest_failed_id,
                    greatest_failed_ctx ? greatest_failed_ctx->original_packets_received : -1,
                    greatest_failed_ctx ? greatest_failed_ctx->num_original_packets : -1);

                FrameData* ctx = get_frame_at_id(ring_buffer, currently_pending_id);
                if (ctx->id == currently_pending_id) {
                    double next_render_staleness = get_timer(&ctx->frame_creation_timer);
                    LOG_INFO("We've been trying to receive Frame %d for %fms.",
                             currently_pending_id, next_render_staleness * MS_IN_SECOND);
                }

                ring_buffer->last_stream_reset_request_id = greatest_failed_id;
            }

            start_timer(&ring_buffer->last_stream_reset_request_timer);
        }
    }
}

NetworkStatistics get_network_statistics(RingBuffer* ring_buffer) {
    // Get the time since last `get_network_statistics` call
    double statistics_time = get_timer(&ring_buffer->network_statistics_timer);

    // Calculate network statistics over that time interval
    NetworkStatistics network_statistics = {0};
    network_statistics.num_nacks_per_second = ring_buffer->num_packets_nacked / statistics_time;
    network_statistics.num_received_packets_per_second =
        ring_buffer->num_packets_received / statistics_time;
    network_statistics.num_rendered_frames_per_second =
        ring_buffer->num_frames_rendered / statistics_time;

    // Reset the ringbuffer statistics' tracking, and the statistics timer
    ring_buffer->num_packets_nacked = 0;
    ring_buffer->num_packets_received = 0;
    ring_buffer->num_frames_rendered = 0;
    start_timer(&ring_buffer->network_statistics_timer);

    // Return the calculated network statistics
    return network_statistics;
}

void destroy_ring_buffer(RingBuffer* ring_buffer) {
    // First, reset the ring buffer
    reset_ring_buffer(ring_buffer);
    // If a frame was held by the renderer then also clear it.
    // (The renderer has already been destroyed when we get here.)
    if (ring_buffer->currently_rendering_id != -1) {
        reset_frame(ring_buffer, &ring_buffer->currently_rendering_frame);
    }
    // free received_frames
    free(ring_buffer->receiving_frames);
    // destroy the block allocator
    destroy_block_allocator(ring_buffer->packet_buffer_allocator);
    // free the ring_buffer
    free(ring_buffer);
}

/*
============================
Private Function Implementations
============================
*/

void init_frame(RingBuffer* ring_buffer, int id, int num_original_indices, int num_fec_indices,
                int prev_frame_num_duplicates) {
    FrameData* frame_data = get_frame_at_id(ring_buffer, id);

    // Confirm that the frame is uninitialized
    FATAL_ASSERT(frame_data->packet_buffer == NULL);
    unsigned char num_entire_frame_nacked = 0;
    if (frame_data->entire_frame_nacked_id == id)
        num_entire_frame_nacked = frame_data->num_entire_frame_nacked;
    // Initialize new framedata
    memset(frame_data, 0, sizeof(*frame_data));
    frame_data->id = id;
    frame_data->packet_buffer = allocate_block(ring_buffer->packet_buffer_allocator);
    frame_data->num_original_packets = num_original_indices;
    frame_data->num_fec_packets = num_fec_indices;
    frame_data->prev_frame_num_duplicate_packets = prev_frame_num_duplicates;
    // If the entire frame was nacked already, then set the packet nack counters and timers
    // appropriately
    if (num_entire_frame_nacked) {
        memset(frame_data->num_times_index_nacked, num_entire_frame_nacked,
               num_original_indices + num_fec_indices);
        for (int i = 0; i < num_original_indices + num_fec_indices; i++) {
            frame_data->last_nacked_timer[i] = frame_data->last_frame_nack_timer;
        }
    }
    start_timer(&frame_data->frame_creation_timer);

    // Initialize FEC-related things, if we need to
    if (num_fec_indices > 0) {
        frame_data->fec_decoder =
            create_fec_decoder(num_original_indices, num_fec_indices, MAX_PACKET_SEGMENT_SIZE);
        frame_data->fec_frame_buffer = allocate_block(ring_buffer->packet_buffer_allocator);
        frame_data->successful_fec_recovery = false;
    }
}

void reset_frame(RingBuffer* ring_buffer, FrameData* frame_data) {
    FATAL_ASSERT(frame_data->packet_buffer != NULL);
    // Free the frame's data
    free_block(ring_buffer->packet_buffer_allocator, frame_data->packet_buffer);
    frame_data->packet_buffer = NULL;
    // Free FEC-related data, if any exists
    if (frame_data->fec_decoder) {
        destroy_fec_decoder(frame_data->fec_decoder);
        frame_data->fec_decoder = NULL;
    }
    if (frame_data->fec_frame_buffer) {
        free_block(ring_buffer->packet_buffer_allocator, frame_data->fec_frame_buffer);
        frame_data->fec_frame_buffer = NULL;
    }
}

void reset_ring_buffer(RingBuffer* ring_buffer) {
    // Note that we do not wipe currently_rendering_frame,
    // since someone else might still be using it
    for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        if (frame_data->packet_buffer != NULL) {
            reset_frame(ring_buffer, frame_data);
        }
        memset(frame_data, 0, sizeof(*frame_data));
    }
    ring_buffer->max_id = -1;
    ring_buffer->min_id = -1;
    ring_buffer->frames_received = 0;
    ring_buffer->last_rendered_id = -1;
}

// Get a pointer to a framebuffer for that id, if such a framebuffer is possible to construct
char* get_framebuffer(RingBuffer* ring_buffer, FrameData* current_frame) {
    if (current_frame->num_fec_packets > 0) {
        if (current_frame->successful_fec_recovery) {
            return current_frame->fec_frame_buffer;
        } else {
            return NULL;
        }
    } else {
        if (current_frame->original_packets_received == current_frame->num_original_packets) {
            return current_frame->packet_buffer;
        } else {
            return NULL;
        }
    }
}

void nack_single_packet(RingBuffer* ring_buffer, int id, int index) {
    ring_buffer->num_packets_nacked++;
    // If a nacking function was passed in, use it
    if (ring_buffer->nack_packet) {
        whist_analyzer_record_nack(ring_buffer->type, id, index);
        ring_buffer->nack_packet(ring_buffer->socket_context, ring_buffer->type, id, index);
    }
}

// Maximum nack bitrate in terms of ratio of total bitrate
#define MAX_NACK_BITRATE_RATIO 0.5

int nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int end_index,
                                     int max_packets_to_nack, double latency) {
    // Something really large, static because this only gets called from one thread
    static char nack_log_buffer[1024 * 32];
    StringBuffer buf;
    string_buffer_init(&buf, nack_log_buffer, sizeof(nack_log_buffer));

    string_buffer_printf(&buf, "NACKing Frame ID %d, Indices ", frame_data->id);

    int num_packets_nacked = 0;
    for (int i = 0; i <= end_index && num_packets_nacked < max_packets_to_nack; i++) {
        // If we can NACK for i, NACK for i
        if (!frame_data->received_indices[i] &&
            (frame_data->num_times_index_nacked[i] == 0 ||
             get_timer(&frame_data->last_nacked_timer[i]) > latency_plus_jitter(latency))) {
            nack_single_packet(ring_buffer, frame_data->id, i);
            string_buffer_printf(&buf, "%s%d", num_packets_nacked == 0 ? "" : ", ", i);
            frame_data->num_times_index_nacked[i]++;
            start_timer(&frame_data->last_nacked_timer[i]);
            num_packets_nacked++;
        }
    }

    if (LOG_NACKING && num_packets_nacked > 0) {
        LOG_INFO("%s", nack_log_buffer);
    }

    return num_packets_nacked;
}

bool try_nacking(RingBuffer* ring_buffer, double latency, int max_unordered_packets,
                 NetworkSettings* network_settings) {
    // We should receive at least one packet for nacking to make sense
    FATAL_ASSERT(ring_buffer->min_id != -1);

    const double burst_interval = 5.0 / MS_IN_SECOND;
    const double avg_interval = 100.0 / MS_IN_SECOND;

    if (get_timer(&ring_buffer->burst_timer) > burst_interval) {
        ring_buffer->burst_counter = 0;
        start_timer(&ring_buffer->burst_timer);
    }
    if (get_timer(&ring_buffer->avg_timer) > avg_interval) {
        ring_buffer->avg_counter = 0;
        start_timer(&ring_buffer->avg_timer);
    }

    // This throttles NACKing to this instantaneous bitrate
    int max_nack_burst_mbps = (int)(network_settings->burst_bitrate * MAX_NACK_BITRATE_RATIO);
    // This is the fundamental NACKing limit
    int max_nack_avg_mbps = (int)(network_settings->video_bitrate * MAX_NACK_BITRATE_RATIO);

    // MAX_MBPS * interval / MAX_PAYLOAD_SIZE is the amount of nack payloads allowed in each
    // interval The XYZ_counter is the amount of packets we've already sent in that interval We
    // subtract the two, to get the max nacks that we're allowed to send at this point in time We
    // min to take the stricter restriction of either burst or average
    // Ensure burst_nacks is atleast 1. If burst_nacks gets truncated to 0, then nacking will never
    // happen
    int burst_nacks =
        max((int)(max_nack_burst_mbps * burst_interval / (MAX_PAYLOAD_SIZE * BITS_IN_BYTE)), 1);
    int avg_nacks = max_nack_avg_mbps * avg_interval / (MAX_PAYLOAD_SIZE * BITS_IN_BYTE);
    int burst_nacks_remaining = burst_nacks - ring_buffer->burst_counter;
    int avg_nacks_remaining = avg_nacks - ring_buffer->avg_counter;
    int max_nacks = (int)min(burst_nacks_remaining, avg_nacks_remaining);
    // Note how the order-of-ops ensures arithmetic is done with double's for higher accuracy

    if (max_nacks <= 0) {
        // We can't nack, so just exit. Also takes care of negative case from above calculation.

        // However, if we also have no average nacks remaining,
        // That means that we've fundamentally saturated NACKing,
        // Rather than simply throttling NACKing
        double saturated_nacking = avg_nacks_remaining <= 0;

        if (ring_buffer->last_nack_possibility && saturated_nacking) {
            if (LOG_NACKING && ring_buffer->type == PACKET_VIDEO) {
                LOG_INFO(
                    "Can't nack anymore! Hit NACK bitrate limit. Try increasing NACK bitrate?");
            }
            ring_buffer->last_nack_possibility = false;
            ring_buffer->num_times_nacking_saturated++;
        }
        // Nacking has failed when avg_nacks has been saturated.
        // If max_nacks has been saturated, that's just burst bitrate distribution
        return !saturated_nacking;
    } else {
        if (!ring_buffer->last_nack_possibility) {
            if (LOG_NACKING && ring_buffer->type == PACKET_VIDEO) {
                LOG_INFO("NACKing is possible again.");
            }
            ring_buffer->last_nack_possibility = true;
        }
    }

    // Track how many nacks we've made this call, to keep it under max_nacks
    int num_packets_nacked = 0;

    // last_missing_frame_nack is strictly increasing so it doesn't need to be throttled
    // Non recovery mode last_packet index is strictly increasing so it doesn't need to be throttled
    // Recovery mode cycles through trying to nack, and we throttle to ~latency,
    // longer during consecutive cycles

    // Nack all the packets we might want to nack about,
    // from oldest to newest, up to max_nacks times
    // If we haven't rendered yet, we'll only nack for the most recent 5 frames when looking for an
    // I-Frame
    for (int id = ring_buffer->last_rendered_id == -1
                      ? max(ring_buffer->max_id - 5, ring_buffer->min_id)
                      : ring_buffer->last_rendered_id + 1;
         id <= ring_buffer->max_id && num_packets_nacked < max_nacks; id++) {
        FrameData* frame_data = get_frame_at_id(ring_buffer, id);
        // If this frame doesn't exist, NACK for the missing frame and then continue
        if (frame_data->id != id) {
            bool nack_frame = false;
            if (frame_data->entire_frame_nacked_id != id) {
                frame_data->entire_frame_nacked_id = id;
                frame_data->num_entire_frame_nacked = 1;
                nack_frame = true;
            } else if (get_timer(&frame_data->last_frame_nack_timer) >
                           latency_plus_jitter(latency) &&
                       frame_data->num_entire_frame_nacked < MAX_PACKET_NACKS) {
                frame_data->num_entire_frame_nacked++;
                nack_frame = true;
            }
            if (nack_frame) {
                if (LOG_NACKING) {
                    LOG_INFO("NACKing for missing Frame ID %d", id);
                }
                nack_single_packet(ring_buffer, id, -1);
                start_timer(&frame_data->last_frame_nack_timer);
                // Assume a frame size that's half the average size, and update the
                // num_packets_nacked accordingly. Usually smaller sized frames are the ones that
                // are lost completely. For average/bigger sized sized frames, one or two packets
                // will usually get through.
                double bytes_per_frame =
                    network_settings->video_bitrate / (MAX_FPS * BITS_IN_BYTE * 2);
                num_packets_nacked += max((int)round(bytes_per_frame / MAX_PAYLOAD_SIZE), 1);
            }
            continue;
        }

        // If this frame has been entirely received, there's nothing to nack for
        if (is_ready_to_render(ring_buffer, id)) {
            continue;
        }

        // =======
        // Go through the frame looking for packets to nack
        // =======

        // Get the last index we received
        int nack_upto_index = 0;
        if (id < ring_buffer->max_id) {
            nack_upto_index = frame_data->num_original_packets - 1;
        } else {
            for (int i = frame_data->num_original_packets - 1; i >= 0; i--) {
                if (frame_data->received_indices[i]) {
                    nack_upto_index = i;
                    break;
                }
            }
        }

        int packets_nacked_this_frame = 0;
        int unordered_packets = max_unordered_packets;
        // If packets from future frames are already received, then adjust unordered_packets
        // accordingly
        for (int future_id = id + 1; future_id < ring_buffer->max_id && unordered_packets > 0;
             future_id++) {
            FrameData* future_frame = get_frame_at_id(ring_buffer, future_id);
            if (future_id == future_frame->id)
                unordered_packets =
                    max(unordered_packets - future_frame->original_packets_received, 0);
        }

        // we nack for packets that are more than unordered_packets "out of order"
        // E.g., if we get 1 2 3 4 5 9, and unordered_packets = 3, then
        // 5 is considered to be too far from 9 to still be unreceived simply due to UDP
        // reordering.
        packets_nacked_this_frame += nack_missing_packets_up_to_index(
            ring_buffer, frame_data, nack_upto_index - unordered_packets,
            max_nacks - num_packets_nacked, latency);
        if (LOG_NACKING && packets_nacked_this_frame > 0) {
            LOG_INFO("~~ Frame ID %d Nacked for %d out-of-order packets", id,
                     packets_nacked_this_frame);
        }
        // Add to total
        num_packets_nacked += packets_nacked_this_frame;
    }

    if (LOG_NACKING && num_packets_nacked > 0) {
        LOG_INFO("Nacked %d/%d packets this Nacking round", num_packets_nacked, max_nacks);
    }

    // Update the counters to track max nack bitrate
    ring_buffer->burst_counter += num_packets_nacked;
    ring_buffer->avg_counter += num_packets_nacked;

    // Nacking succeeded
    return true;
}
