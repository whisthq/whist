/*
============================
Includes
============================
*/

#include "ringbuffer.h"
#include <assert.h>
#include <whist/utils/fec.h>

/*
============================
Defines
============================
*/

#define MAX_RING_BUFFER_SIZE 500
#define MAX_VIDEO_PACKETS 500
#define MAX_AUDIO_PACKETS 3
#define MAX_UNORDERED_PACKETS 10

#define MAX_PACKETS (get_num_fec_packets(max(MAX_VIDEO_PACKETS, MAX_AUDIO_PACKETS), MAX_FEC_RATIO))

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
 */
void init_frame(RingBuffer* ring_buffer, int id, int num_original_indices, int num_fec_indices);

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
                                            int end_index, int max_packets_to_nack);

/**
 * @brief If any packets are still missing, and it's been too long, try nacking for them.
 *        Ideally, this gets called quite rapidly, it has internal timers to throttle nacks.
 *        The more rapidly the better, just need to balance CPU usage, 5-10ms should be fine.
 *
 * @param ring_buffer The ring buffer to try nacking with
 *
 * @param latency The round-trip latency of the connection. Helpful with nacking logic
 *
 * @returns     True if nacking succeded,
 *              False if we've bandwidth saturated our ability to nack.
 */
static bool try_nacking(RingBuffer* ring_buffer, double latency);

// TODO: document this
char* get_framebuffer(RingBuffer* ring_buffer, FrameData* current_frame);

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
        frame_data->id = -1;
    }

    // determine largest frame size, including the WhistPacket header
    ring_buffer->largest_frame_size = sizeof(WhistPacket) - MAX_PAYLOAD_SIZE + max_frame_size;

    ring_buffer->packet_buffer_allocator = create_block_allocator(ring_buffer->largest_frame_size);
    ring_buffer->currently_rendering_id = -1;
    ring_buffer->last_rendered_id = -1;
    ring_buffer->last_missing_frame_nack = -1;

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

    return ring_buffer;
}

int ring_buffer_receive_segment(RingBuffer* ring_buffer, WhistSegment* segment) {
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

    // If segment_id != frame_data->id, handle the situation
    if (segment_id < frame_data->id) {
        // This packet must be from a very stale frame,
        // because the current ringbuffer occupant already contains packets with a newer ID in it
        LOG_WARNING("Very stale packet (ID %d) received, current ringbuffer occupant's ID %d",
                    segment_id, frame_data->id);
        return -1;
    } else if (segment_id <= ring_buffer->currently_rendering_id) {
        // This packet won't help us render any new packets,
        // So we can safely just ignore it
        return 0;
    } else if (segment_id > frame_data->id) {
        // This packet is newer than the resident,
        // so it's time to overwrite the resident if such a resident exists
        if (frame_data->id != -1) {
            if (frame_data->id > ring_buffer->currently_rendering_id) {
                // We have received a packet which will overwrite a frame that needs to be rendered
                // in the future. In other words, the ring buffer is full, so we should wipe the
                // whole ring buffer.
                LOG_WARNING(
                    "We received a packet with %s Frame ID %d, that is trying to overwrite Frame "
                    "ID "
                    "%d!\n"
                    "But we can't overwrite that frame, since our renderer has only gotten to ID "
                    "%d!\n"
                    "Resetting the entire ringbuffer...",
                    type == PACKET_VIDEO ? "video" : "audio", segment_id, frame_data->id,
                    ring_buffer->currently_rendering_id);
                // TODO: log a FPS skip
                reset_ring_buffer(ring_buffer);
            } else {
                // Here, the frame is older than where our renderer is,
                // So we can just reset the undesired frame
                LOG_ERROR(
                    "Trying to allocate Frame ID %d, but Frame ID %d has not been destroyed yet!",
                    segment_id, frame_data->id);
                reset_frame(ring_buffer, frame_data);
            }
        }

        // Initialize the frame now, so that it can hold the packet we just received
        int num_original_packets = num_indices - num_fec_indices;
        init_frame(ring_buffer, segment_id, num_original_packets, num_fec_indices);

        // Update the ringbuffer's max id, with this new frame's ID
        ring_buffer->max_id = max(ring_buffer->max_id, frame_data->id);
    }

    // Now, the frame_data should be ready to accept the packet
    FATAL_ASSERT(segment_id == frame_data->id);

    // Verify that the packet metadata matches frame_data metadata
    FATAL_ASSERT(frame_data->num_fec_packets == num_fec_indices);
    FATAL_ASSERT(frame_data->num_original_packets + frame_data->num_fec_packets == num_indices);

    // LOG the the nacking situation
    if (segment->is_a_nack) {
        // Server simulates a nack for audio all the time. Hence log only for video.
        if (type == PACKET_VIDEO) {
            if (!frame_data->received_indices[segment_index]) {
                LOG_INFO("NACK for video ID %d, Index %d received!", segment_id, segment_index);
            } else {
                LOG_INFO("NACK for video ID %d, Index %d received, but didn't need it.", segment_id,
                         segment_index);
            }
        }
    } else {
        // Reset timer since the last time we received a non-nack packet
        start_timer(&frame_data->last_nonnack_packet_timer);
        if (frame_data->num_times_index_nacked[segment_index] > 0) {
            LOG_INFO("Received original %s ID %d, Index %d, but we had NACK'ed for it.",
                     type == PACKET_VIDEO ? "video" : "audio", segment_id, segment_index);
        }
    }

    // If we have already received this packet anyway, just drop this packet
    if (frame_data->received_indices[segment_index]) {
        // The only way it should possible to receive a packet twice, is if nacking got involved
        if (type == PACKET_VIDEO && frame_data->num_times_index_nacked[segment_index] == 0) {
            LOG_ERROR(
                "We received a video packet (ID %d / index %d) twice, but we had never nacked for "
                "it?",
                segment_id, segment_index);
            return -1;
        }
        return 0;
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

    // Copy the packet's payload into the correct location in frame_data's frame buffer
    int buffer_offset = segment_index * MAX_PACKET_SEGMENT_SIZE;
    if (buffer_offset + segment_size >= ring_buffer->largest_frame_size) {
        LOG_ERROR("Packet payload too large for frame buffer! Dropping the packet...");
        return -1;
    }
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

        // Using the newly registered packet, try to decode the frame using FEC
        int frame_size =
            fec_get_decoded_buffer(frame_data->fec_decoder, frame_data->fec_frame_buffer);

        // If we were able to successfully decode the frame, mark it as such!
        if (frame_size >= 0) {
            if (frame_data->original_packets_received < frame_data->num_original_packets) {
                LOG_INFO("Successfully recovered %d/%d Packet %d, using %d FEC packets",
                         frame_data->original_packets_received, frame_data->num_original_packets,
                         frame_data->id, frame_data->fec_packets_received);
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

    return 0;
}

FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id) {
    return &ring_buffer->receiving_frames[id % ring_buffer->ring_buffer_size];
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

    // Move frame from current_frame, to currently_rendering_frame
    FrameData* current_frame = get_frame_at_id(ring_buffer, id);
    ring_buffer->currently_rendering_id = id;
    ring_buffer->currently_rendering_frame = *current_frame;

    // Invalidate the current_frame, without deallocating its data with reset_frame,
    // Since currently_rendering_frame now owns that data
    memset(current_frame, 0, sizeof(*current_frame));
    current_frame->id = -1;

    // Set the framebuffer pointer of the currently rendering frame
    ring_buffer->currently_rendering_frame.frame_buffer =
        get_framebuffer(ring_buffer, &ring_buffer->currently_rendering_frame);

    // Track for statistics
    ring_buffer->num_frames_rendered++;

    // Return the currently rendering frame
    return &ring_buffer->currently_rendering_frame;
}

void reset_stream(RingBuffer* ring_buffer, int id) {
    /*
        "Skip" to the frame at ID id by setting last_rendered_id = id - 1 (if the skip is valid).
    */

    FATAL_ASSERT(0 <= id);

    // Check that we're not trying to reset to a frame that's been rendered already
    if (id <= ring_buffer->last_rendered_id) {
        LOG_INFO("Received stale stream reset request - told to skip to ID %d but already at ID %d",
                 id, ring_buffer->last_rendered_id);
    } else {
        if (ring_buffer->last_rendered_id != -1) {
            // Loudly log if we're dropping frames
            LOG_INFO("Skipping from frame %d to frame %d", ring_buffer->last_rendered_id, id);
            // Drop frames up until id
            // We also use id - frames_received, to prevent printing a jump from 0 to 50,000
            for (int i =
                     max(ring_buffer->last_rendered_id + 1, id - ring_buffer->frames_received + 1);
                 i < id; i++) {
                FrameData* frame_data = get_frame_at_id(ring_buffer, i);
                if (frame_data->id == i) {
                    // Only verbosely log reset_stream for VIDEO frames
                    if (ring_buffer->type == PACKET_VIDEO) {
                        LOG_WARNING("Frame dropped with ID %d: %d/%d", i,
                                    frame_data->original_packets_received,
                                    frame_data->num_original_packets);
                        for (int j = 0; j < frame_data->num_original_packets; j++) {
                            if (!frame_data->received_indices[j]) {
                                LOG_WARNING("Did not receive ID %d, Index %d", i, j);
                            }
                        }
                    }
                    reset_frame(ring_buffer, frame_data);
                } else if (frame_data->id != -1) {
                    LOG_WARNING("Bad ID? %d instead of %d", frame_data->id, i);
                    reset_frame(ring_buffer, frame_data);
                }
            }
        }
        ring_buffer->last_rendered_id = id - 1;
    }
}

void try_recovering_missing_packets_or_frames(RingBuffer* ring_buffer, double latency) {
    // this nacks for missing packets
    // and sends stream reset requests if needed
#define STREAM_RESET_REQUEST_INTERVAL_MS 5.0
#define MAX_UNSYNCED_FRAMES 4
#define MAX_TO_RENDER_STALENESS 100.0
    // Try to nack for any missing packets
    bool nacking_succeeded = try_nacking(ring_buffer, latency);

    // Get how stale the frame we're trying to render is
    double next_to_render_staleness = -1.0;
    int next_render_id = ring_buffer->last_rendered_id + 1;
    FrameData* ctx = get_frame_at_id(ring_buffer, next_render_id);
    if (ctx->id == next_render_id) {
        next_to_render_staleness = get_timer(&ctx->frame_creation_timer);
    }

    // If nacking has failed to recover the packets we need,
    if (!nacking_succeeded
        // Or if we're more than MAX_UNSYNCED_FRAMES frames out-of-sync,
        || ring_buffer->max_id > ring_buffer->last_rendered_id + MAX_UNSYNCED_FRAMES
        // Or we've spent MAX_TO_RENDER_STALENESS trying to render this one frame
        || (next_to_render_staleness > MAX_TO_RENDER_STALENESS / MS_IN_SECOND)) {
        // THEN, we should send an iframe request

        // Throttle the requests to prevent network upload saturation, however
        if (get_timer(&ring_buffer->last_stream_reset_request_timer) >
            STREAM_RESET_REQUEST_INTERVAL_MS / MS_IN_SECOND) {
            ring_buffer->request_stream_reset(ring_buffer->socket_context, ring_buffer->type,
                                              max(next_render_id, ring_buffer->max_id - 1));
        }

        /*
        LOG_INFO(
                "The most recent ID %d is %d frames ahead of the most recently rendered frame, "
                "and the frame we're trying to render has been alive for %fms. "
                "A stream reset is now being requested to catch-up.",
                ring_buffer->max_id,
                ring_buffer->max_id - ring_buffer->last_rendered_id,
                next_to_render_staleness * MS_IN_SECOND);
                */
        start_timer(&ring_buffer->last_stream_reset_request_timer);
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

void init_frame(RingBuffer* ring_buffer, int id, int num_original_indices, int num_fec_indices) {
    FrameData* frame_data = get_frame_at_id(ring_buffer, id);

    // Confirm that the frame is uninitialized
    FATAL_ASSERT(frame_data->id == -1);

    // Initialize new framedata
    memset(frame_data, 0, sizeof(*frame_data));
    frame_data->id = id;
    frame_data->packet_buffer = allocate_block(ring_buffer->packet_buffer_allocator);
    frame_data->original_packets_received = 0;
    frame_data->fec_packets_received = 0;
    frame_data->num_original_packets = num_original_indices;
    frame_data->num_fec_packets = num_fec_indices;
    frame_data->received_indices = safe_malloc(MAX_PACKETS * sizeof(bool));
    frame_data->num_times_index_nacked = safe_malloc(MAX_PACKETS * sizeof(int));
    memset(frame_data->received_indices, 0, MAX_PACKETS * sizeof(bool));
    memset(frame_data->num_times_index_nacked, 0, MAX_PACKETS * sizeof(int));
    frame_data->last_nacked_index = -1;
    frame_data->num_times_nacked = 0;
    frame_data->type = ring_buffer->type;
    frame_data->recovery_mode = false;
    start_timer(&frame_data->frame_creation_timer);
    start_timer(&frame_data->last_nacked_timer);

    // Initialize FEC-related things, if we need to
    if (num_fec_indices > 0) {
        frame_data->fec_decoder =
            create_fec_decoder(num_original_indices, num_fec_indices, MAX_PAYLOAD_SIZE);
        frame_data->fec_frame_buffer = allocate_block(ring_buffer->packet_buffer_allocator);
        frame_data->successful_fec_recovery = false;
    }
}

void reset_frame(RingBuffer* ring_buffer, FrameData* frame_data) {
    FATAL_ASSERT(frame_data->id != -1);

    // Free the frame's data
    free_block(ring_buffer->packet_buffer_allocator, frame_data->packet_buffer);
    free(frame_data->received_indices);
    free(frame_data->num_times_index_nacked);

    // Free FEC-related data, if any exists
    if (frame_data->num_fec_packets > 0) {
        destroy_fec_decoder(frame_data->fec_decoder);
        free_block(ring_buffer->packet_buffer_allocator, frame_data->fec_frame_buffer);
    }

    // Mark as uninitialized
    memset(frame_data, 0, sizeof(*frame_data));
    frame_data->id = -1;
}

void reset_ring_buffer(RingBuffer* ring_buffer) {
    // Note that we do not wipe currently_rendering_frame,
    // since someone else might still be using it
    for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        if (frame_data->id != -1) {
            reset_frame(ring_buffer, frame_data);
        }
    }
    ring_buffer->max_id = -1;
    ring_buffer->frames_received = 0;
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
        ring_buffer->nack_packet(ring_buffer->socket_context, ring_buffer->type, id, index);
    }
}

// The max number of times we can nack a packet: limited to 2 times right now so that we don't get
// stuck on a packet that never arrives
#define MAX_PACKET_NACKS 2
// Maximum amount of mbps that can be used by nacking
// This is calculated per 100ms interval
#define MAX_NACK_AVG_MBPS 2200000
// Maximum burst mbps that can be used by nacking
// This is calculated per 5ms interval
#define MAX_NACK_BURST_MBPS 4800000

int nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int end_index,
                                     int max_packets_to_nack) {
    /*
        Nack up to 1 missing packet up to index

        Arguments:
            ring_buffer (RingBuffer*): ring buffer missing packets
            frame_data (FrameData*): frame missing packets
            end_index (int): the index up to which to nack missing packets
            max_packets_to_nack (int): Most amount of packets we want to nack for

        Returns:
            The number of packets Nacked
    */

    // Note that an invalid last_nacked_index is set to -1, correctly starting us at 0
    int start_index = frame_data->last_nacked_index + 1;

    // Something really large, static because this only gets called from one thread
    static char nack_log_buffer[1024 * 32];
    int log_len = 0;
    log_len += snprintf(nack_log_buffer + log_len, sizeof(nack_log_buffer) - log_len,
                        "NACKing Frame ID %d, Indices ", frame_data->id);

    int num_packets_nacked = 0;
    for (int i = start_index; i <= end_index && num_packets_nacked < max_packets_to_nack; i++) {
        if (!frame_data->received_indices[i] &&
            frame_data->num_times_index_nacked[i] < MAX_PACKET_NACKS) {
            nack_single_packet(ring_buffer, frame_data->id, i);
            log_len += snprintf(nack_log_buffer + log_len, sizeof(nack_log_buffer) - log_len,
                                "%s%d", num_packets_nacked == 0 ? "" : ", ", i);
            frame_data->num_times_index_nacked[i]++;
            frame_data->last_nacked_index = i;
            num_packets_nacked++;
        }
    }

    if (num_packets_nacked > 0) {
        LOG_INFO("%s", nack_log_buffer);
    }

    return num_packets_nacked;
}

bool try_nacking(RingBuffer* ring_buffer, double latency) {
    if (ring_buffer->max_id == -1) {
        // Don't nack if we haven't received anything yet
        // Return true, our nacking vacuously succeeded
        return true;
    }

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

    // MAX_MBPS * interval / MAX_PAYLOAD_SIZE is the amount of nack payloads allowed in each
    // interval The XYZ_counter is the amount of packets we've already sent in that interval We
    // subtract the two, to get the max nacks that we're allowed to send at this point in time We
    // min to take the stricter restriction of either burst or average
    int max_nacks_remaining =
        MAX_NACK_BURST_MBPS * burst_interval / MAX_PAYLOAD_SIZE - ring_buffer->burst_counter;
    int avg_nacks_remaining =
        MAX_NACK_AVG_MBPS * avg_interval / MAX_PAYLOAD_SIZE - ring_buffer->avg_counter;
    int max_nacks = (int)min(max_nacks_remaining, avg_nacks_remaining);
    // Note how the order-of-ops ensures arithmetic is done with double's for higher accuracy

    if (max_nacks <= 0) {
        // We can't nack, so just exit. Also takes care of negative case from above calculation.
        if (ring_buffer->last_nack_possibility) {
            LOG_INFO("Can't nack anymore! Hit NACK bitrate limit. Try increasing NACK bitrate?");
            ring_buffer->last_nack_possibility = false;
        }
        // Nacking has failed when avg_nacks has been saturated.
        // If max_nacks has been saturated, that's just burst bitrate distribution
        return avg_nacks_remaining > 0;
    } else {
        if (!ring_buffer->last_nack_possibility) {
            LOG_INFO("NACKing is possible again.");
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
    for (int id = ring_buffer->last_rendered_id == -1 ? max(ring_buffer->max_id - 5, 1)
                                                      : ring_buffer->last_rendered_id + 1;
         id <= ring_buffer->max_id && num_packets_nacked < max_nacks; id++) {
        FrameData* frame_data = get_frame_at_id(ring_buffer, id);
        // If this frame doesn't exist, skip it
        if (frame_data->id != id) {
            // If we've received nothing from a frame before max_id,
            // let's try nacking for index 0 of it
            if (ring_buffer->last_missing_frame_nack < id) {
                // Nack the first set of indices of the missing frame
                // Frames 10-20 packets in size, we'd like to recover in one RTT,
                // But we don't want to try to recover 200 packet frames so quickly
                for (int index = 0; index <= 20; index++) {
                    nack_single_packet(ring_buffer, id, index);
                    // Track out-of-bounds nacks anyway in bitrate,
                    // To ensure the upper-bound on bandwidth usage
                    num_packets_nacked++;
                }
                LOG_INFO("NACKing for missing Frame ID %d", id);
                ring_buffer->last_missing_frame_nack = id;
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
        int last_packet_received = 0;
        for (int i = frame_data->num_original_packets - 1; i >= 0; i--) {
            if (frame_data->received_indices[i]) {
                last_packet_received = i;
                break;
            }
        }

        // If too much time has passed since the last packet received,
        // we swap into *recovery mode*, since something is probably wrong with this packet
        if ((id < ring_buffer->max_id ||
             get_timer(&frame_data->last_nonnack_packet_timer) > 0.2 * latency) &&
            !frame_data->recovery_mode) {
#if LOG_NACKING
            LOG_INFO("Too long since last non-nack packet from ID %d. Entering recovery mode...",
                     id);
#endif
            frame_data->recovery_mode = true;
        }

        // Track packets nacked this frame
        int packets_nacked_this_frame = 0;

        if (!frame_data->recovery_mode) {
            // During *normal nacking mode*,
            // we nack for packets that are more than MAX_UNORDERED_PACKETS "out of order"
            // E.g., if we get 1 2 3 4 5 9, and MAX_UNORDERED_PACKETS = 3, then
            // 5 is considered to be too far from 9 to still be unreceived simply due to UDP
            // reordering.
            packets_nacked_this_frame += nack_missing_packets_up_to_index(
                ring_buffer, frame_data, last_packet_received - MAX_UNORDERED_PACKETS,
                max_nacks - num_packets_nacked);
#if LOG_NACKING
            if (packets_nacked_this_frame > 0) {
                LOG_INFO("~~ Frame ID %d Nacked for %d out-of-order packets", id,
                         packets_nacked_this_frame);
            }
#endif
        } else {
            // *Recovery mode* means something is wrong with the network,
            // and we should keep trying to nack for those missing packets.
            // On the first round, we finish up the work that the *normal nacking mode* did,
            // i.e. we nack for everything after last_packet_received - MAX_UNORDERED_PACKETS.
            // After an additional 1.2 * latency, we send another round of nacks
            if (get_timer(&frame_data->last_nacked_timer) >
                1.2 * latency * frame_data->num_times_nacked) {
#if LOG_NACKING
                LOG_INFO("Attempting to recover Frame ID %d, %d/%d indices received.", id,
                         frame_data->packets_received, frame_data->num_packets);
#endif
                packets_nacked_this_frame = nack_missing_packets_up_to_index(
                    ring_buffer, frame_data, frame_data->num_original_packets - 1,
                    max_nacks - num_packets_nacked);
                if (frame_data->last_nacked_index == frame_data->num_original_packets - 1) {
                    frame_data->last_nacked_index = -1;
                    start_timer(&frame_data->last_nacked_timer);

                    frame_data->num_times_nacked++;
#if LOG_NACKING
                    LOG_INFO("- Done with Nacking cycle #%d, restarting cycle",
                             frame_data->num_times_nacked);
#endif
                }
#if LOG_NACKING
                LOG_INFO("~~ Frame ID %d Nacked for %d packets in recovery mode", id,
                         packets_nacked_this_frame);
#endif
            }
        }

        // Add to total
        num_packets_nacked += packets_nacked_this_frame;
    }

#if LOG_NACKING
    if (num_packets_nacked > 0) {
        LOG_INFO("Nacked %d/%d packets this Nacking round", num_packets_nacked, max_nacks);
    }
#endif

    // Update the counters to track max nack bitrate
    ring_buffer->burst_counter += num_packets_nacked;
    ring_buffer->avg_counter += num_packets_nacked;

    // Nacking succeeded
    return true;
}
