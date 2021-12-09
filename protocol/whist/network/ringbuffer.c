#include "ringbuffer.h"
#include <assert.h>

#define MAX_RING_BUFFER_SIZE 500
#define MAX_VIDEO_PACKETS 500
#define MAX_AUDIO_PACKETS 3
#define MAX_UNORDERED_PACKETS 10

void reset_ring_buffer(RingBuffer* ring_buffer);
void init_frame(RingBuffer* ring_buffer, int id, int num_indices);
void allocate_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);
void destroy_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);

void reset_ring_buffer(RingBuffer* ring_buffer) {
    /*
        Reset the members of ring_buffer except type and size and the elements
        used for bitrate calculation.
    */
    // LOG_DEBUG("Currently rendering ID: %d", ring_buffer->currently_rendering_id);
    // wipe all frames
    for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        reset_frame(ring_buffer, frame_data);
    }
    ring_buffer->max_id = -1;
    ring_buffer->frames_received = 0;
}

void reset_bitrate_stat_members(RingBuffer* ring_buffer) {
    /*
        Reset all accumulators used for calculating bitrate stats to 0.

        NOTE: This is separate from `reset_ring_buffer` because the
        `calculate_statistics` function takes care of resting these
        members when necessary.
    */

    ring_buffer->num_packets_nacked = 0;
    ring_buffer->num_packets_received = 0;
    ring_buffer->num_frames_skipped = 0;
    ring_buffer->num_frames_rendered = 0;
}

RingBuffer* init_ring_buffer(WhistPacketType type, int ring_buffer_size, NackPacketFn nack_packet) {
    /*
        Initialize the ring buffer; malloc space for all the frames and set their IDs to -1.

        Arguments:
            type (WhistPacketType): audio or video
            ring_buffer_size (int): Desired size of the ring buffer. If the size exceeds
       MAX_RING_BUFFER_SIZE, no ring buffer is returned.

        Returns:
            (RingBuffer*): pointer to the created ring buffer
            */
    if (ring_buffer_size > MAX_RING_BUFFER_SIZE) {
        LOG_ERROR("Requested ring buffer size %d too large - ensure size is at most %d",
                  ring_buffer_size, MAX_RING_BUFFER_SIZE);
        return NULL;
    }
    RingBuffer* ring_buffer = safe_malloc(sizeof(RingBuffer));
    if (!ring_buffer) {
        return NULL;
    }

    ring_buffer->type = type;
    ring_buffer->ring_buffer_size = ring_buffer_size;
    ring_buffer->receiving_frames = safe_malloc(ring_buffer_size * sizeof(FrameData));
    ring_buffer->nack_packet = nack_packet;
    if (!ring_buffer->receiving_frames) {
        return NULL;
    }
    ring_buffer->largest_num_packets =
        ring_buffer->type == PACKET_VIDEO ? MAX_VIDEO_PACKETS : MAX_AUDIO_PACKETS;
    // allocate data for frames
    for (int i = 0; i < ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        frame_data->frame_buffer = NULL;
        int indices_array_size = ring_buffer->largest_num_packets * sizeof(bool);
        int nacked_array_size = ring_buffer->largest_num_packets * sizeof(int);
        frame_data->received_indices = safe_malloc(indices_array_size);
        frame_data->nacked_indices = safe_malloc(nacked_array_size);
    }

    // determine largest frame size
    ring_buffer->largest_frame_size =
        ring_buffer->type == PACKET_VIDEO ? LARGEST_VIDEOFRAME_SIZE : LARGEST_AUDIOFRAME_SIZE;

    ring_buffer->frame_buffer_allocator = create_block_allocator(ring_buffer->largest_frame_size);
    ring_buffer->currently_rendering_id = -1;
    ring_buffer->last_rendered_id = -1;
    ring_buffer->last_missing_frame_nack = -1;

    // set all additional metadata for frames and ring buffer
    reset_ring_buffer(ring_buffer);
    reset_bitrate_stat_members(ring_buffer);

    return ring_buffer;
}

FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id) {
    /*
        Retrieve the frame in ring_buffer of ID id. Currently, does not check that the ID of the
       retrieved frame is actually the desired ID.

        Arguments:
            ring_buffer (RingBuffer*): the ring buffer holding the frame
            id (int): desired frame ID

        Returns:
            (FrameData*): Pointer to FrameData at the correct index
            */
    return &ring_buffer->receiving_frames[id % ring_buffer->ring_buffer_size];
}

void allocate_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data) {
    /*
        Helper function to allocate the frame buffer which will hold UDP packets. We use a block
       allocator because we're going to be constantly freeing frames.

        Arguments:
            ring_buffer (RingBuffer*): Ring buffer holding frame
            frame_data (FrameData*): Frame whose frame buffer we want to allocate
        */
    if (frame_data->frame_buffer == NULL) {
        frame_data->frame_buffer = allocate_block(ring_buffer->frame_buffer_allocator);
    }
}

void init_frame(RingBuffer* ring_buffer, int id, int num_indices) {
    /*
        Initialize a frame with num_indices indices and ID id. This allocates the frame buffer and
        the arrays we use for metadata.

        Arguments:
            ring_buffer (RingBuffer*): ring buffer containing the frame
            id (int): desired frame ID
            num_indices (int): number of indices in the frame
            */
    FrameData* frame_data = get_frame_at_id(ring_buffer, id);
    int indices_array_size = ring_buffer->largest_num_packets * sizeof(bool);
    int nacked_array_size = ring_buffer->largest_num_packets * sizeof(int);
    // initialize new framedata
    frame_data->id = id;
    allocate_frame_buffer(ring_buffer, frame_data);
    frame_data->packets_received = 0;
    frame_data->num_packets = num_indices;
    memset(frame_data->received_indices, 0, indices_array_size);
    memset(frame_data->nacked_indices, 0, nacked_array_size);
    frame_data->last_nacked_index = -1;
    frame_data->num_times_nacked = 0;
    frame_data->frame_size = 0;
    frame_data->type = ring_buffer->type;
    frame_data->recovery_mode = false;
    start_timer(&frame_data->frame_creation_timer);
    start_timer(&frame_data->last_nacked_timer);
}

void reset_frame(RingBuffer* ring_buffer, FrameData* frame_data) {
    /*
       Reset the frame's frame buffer and its metadata. Useful for when we're skipping frames and
       don't want to leave stale frames in the buffer.

        Arguments:
            ring_buffer (RingBuffer*): ring buffer containing the frame
            frame_data (FrameData*): frame to reset
    */

    destroy_frame_buffer(ring_buffer, frame_data);
    frame_data->id = -1;
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
        // destroy the frame buffer of the currently rendering frame
        destroy_frame_buffer(ring_buffer, &ring_buffer->currently_rendering_frame);
    }
    // set the currently rendering ID and frame
    ring_buffer->currently_rendering_id = id;
    FrameData* current_frame = get_frame_at_id(ring_buffer, id);
    if (current_frame->id != id) {
        LOG_FATAL("The Frame ID %d does not exist, got %d instead", id, current_frame->id);
    }
    if (current_frame->packets_received != current_frame->num_packets) {
        LOG_FATAL("Tried to call set_rendering on an incomplete packet %d!", id);
    }
    ring_buffer->currently_rendering_frame = *current_frame;
    // clear the current frame's data
    current_frame->frame_buffer = NULL;
    reset_frame(ring_buffer, current_frame);

    return &ring_buffer->currently_rendering_frame;
}

int receive_packet(RingBuffer* ring_buffer, WhistPacket* packet) {
    /*
        Process a WhistPacket and add it to the ring buffer. If the packet belongs to an existing
        frame, copy its data into the frame; if it belongs to a new frame, initialize the frame and
        copy data. Nack for missing packets (of the packet's frame) and missing frames (before the
        current frame).

        Arguments:
            ring_buffer (RingBuffer*): ring buffer to place the packet in
            packet (WhistPacket*): UDP packet for either audio or video

        Returns:
            (int): 1 if we overwrote a valid frame, 0 on success, -1 on failure
            */
    FrameData* frame_data = get_frame_at_id(ring_buffer, packet->id);

    ring_buffer->num_packets_received++;

    int num_overwritten_frames = 0;
    // This packet is old, because the current resident already contains packets with a newer ID in
    // it
    if (packet->id < frame_data->id) {
        LOG_INFO("Old packet (ID %d) received, previous ID %d", packet->id, frame_data->id);
        return -1;
        // This packet is newer than the resident, so it's time to overwrite the resident
    } else if (packet->id > frame_data->id) {
        if (frame_data->id != -1) {
            // We can overwrite no matter what, since the currently rendering frame has already been
            // copied to currently_rendering_frame.
            num_overwritten_frames = 1;
            if (frame_data->id > ring_buffer->currently_rendering_id) {
                // We have received a packet which will overwrite a frame that needs to be rendered
                // in the future. In other words, the ring buffer is full, so we should wipe the
                // whole ring buffer.
                LOG_INFO(
                    "We received a packet with ID %d, that will overwrite the frame with ID %d!",
                    packet->id, frame_data->id);
                LOG_INFO(
                    "We can't overwrite that frame, since our renderer has only gotten to ID "
                    "%d!",
                    ring_buffer->currently_rendering_id);
                num_overwritten_frames = packet->id - ring_buffer->currently_rendering_id - 1;
                reset_ring_buffer(ring_buffer);
            }
        }
        // Now, we can overwrite with no other concerns
        init_frame(ring_buffer, packet->id, packet->num_indices);
    }

    // check if we have nacked for the packet
    // TODO: log video vs audio
    bool already_logged_about_it = false;
    if (packet->is_a_nack) {
        if (!frame_data->received_indices[packet->index]) {
            LOG_INFO("NACK for ID %d, Index %d received!", packet->id, packet->index);
            already_logged_about_it = true;
        } else {
            LOG_INFO("NACK for ID %d, Index %d received, but didn't need it.", packet->id,
                     packet->index);
            already_logged_about_it = true;
        }
    } else {
        // Reset timer since the last time we received a real non-nack packet
        start_timer(&frame_data->last_nonnack_packet_timer);
        if (frame_data->nacked_indices[packet->index] > 0) {
            LOG_INFO("Received original ID %d, Index %d, but we had NACK'ed for it.", packet->id,
                     packet->index);
            already_logged_about_it = true;
        }
    }

    // If we have already received the packet, there is nothing to do
    if (frame_data->received_indices[packet->index]) {
        if (!already_logged_about_it) {
            // It shouldn't be possible to receive the frame twice, without nacking for it
            LOG_ERROR("Duplicate of ID %d, Index %d received", packet->id, packet->index);
        }
        return -1;
    }

    // Update framedata metadata + buffer
    ring_buffer->max_id = max(ring_buffer->max_id, frame_data->id);
    frame_data->received_indices[packet->index] = true;
    frame_data->packets_received++;
    // Copy the packet data
    int place = packet->index * MAX_PAYLOAD_SIZE;
    if (place + packet->payload_size >= ring_buffer->largest_frame_size) {
        LOG_ERROR("Packet payload too large for frame buffer!");
        return -1;
    }
    memcpy(frame_data->frame_buffer + place, packet->data, packet->payload_size);
    frame_data->frame_size += packet->payload_size;

    if (frame_data->packets_received == frame_data->num_packets) {
        ring_buffer->frames_received++;
    }
    return num_overwritten_frames;
}

void nack_single_packet(RingBuffer* ring_buffer, int id, int index) {
    ring_buffer->num_packets_nacked++;
    // If a nacking function was passed in, use it
    if (ring_buffer->nack_packet) {
        ring_buffer->nack_packet(ring_buffer->type, id, index);
    }
}

void nack_bitarray_packets(RingBuffer* ring_buffer, int id, int start_index, BitArray* bit_arr) {
    /*
        Nack the packets at ID id and start_index start_index.

        Arguments:
            ring_buffer (RingBuffer*): the ring buffer waiting for the packet
            id (int): Frame ID of the packet
            start_index (int): index of the first packet to be NACKed
            bit_arr (BitArray): the bit array with the indexes of the packets to NACK.
            */

    LOG_INFO("NACKing with bit array for Packets with ID %d, Starting Index %d", id, start_index);
    WhistClientMessage fcmsg = {0};
    fcmsg.type = MESSAGE_BITARRAY_NACK;
    fcmsg.bitarray_nack.type = ring_buffer->type;
    fcmsg.bitarray_nack.id = id;
    fcmsg.bitarray_nack.index = start_index;
    memset(fcmsg.bitarray_nack.ba_raw, 0, BITS_TO_CHARS(bit_arr->numBits));
    memcpy(fcmsg.bitarray_nack.ba_raw, bit_array_get_bits(bit_arr),
           BITS_TO_CHARS(bit_arr->numBits));
    fcmsg.bitarray_nack.numBits = bit_arr->numBits;

    for (unsigned int i = 0; i < bit_arr->numBits; i++) {
        if (bit_array_test_bit(bit_arr, i)) {
            ring_buffer->num_packets_nacked++;
        }
    }

    // send_fcmsg(&fcmsg);
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
        if (!frame_data->received_indices[i] && frame_data->nacked_indices[i] < MAX_PACKET_NACKS) {
            nack_single_packet(ring_buffer, frame_data->id, i);
            log_len += snprintf(nack_log_buffer + log_len, sizeof(nack_log_buffer) - log_len,
                                "%s%d", num_packets_nacked == 0 ? "" : ", ", i);
            frame_data->nacked_indices[i]++;
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
    if (ring_buffer->last_rendered_id == -1) {
        ring_buffer->last_rendered_id = ring_buffer->max_id - 1;
    }

    static bool first_call = true;
    static clock burst_timer;
    static clock avg_timer;
    static int burst_counter;
    static int avg_counter;

    double burst_interval = 5.0 / MS_IN_SECOND;
    double avg_interval = 100.0 / MS_IN_SECOND;

    if (first_call || get_timer(burst_timer) > burst_interval) {
        burst_counter = 0;
        start_timer(&burst_timer);
    }
    if (first_call || get_timer(avg_timer) > avg_interval) {
        avg_counter = 0;
        start_timer(&avg_timer);
        first_call = false;
    }

    // MAX_MBPS * interval / MAX_PAYLOAD_SIZE is the amount of nack payloads allowed in each
    // interval The XYZ_counter is the amount of packets we've already sent in that interval We
    // subtract the two, to get the max nacks that we're allowed to send at this point in time We
    // min to take the stricter restriction of either burst or average
    int max_nacks_remaining =
        MAX_NACK_BURST_MBPS * burst_interval / MAX_PAYLOAD_SIZE - burst_counter;
    int avg_nacks_remaining = MAX_NACK_AVG_MBPS * avg_interval / MAX_PAYLOAD_SIZE - avg_counter;
    int max_nacks = (int)min(max_nacks_remaining, avg_nacks_remaining);
    // Note how the order-of-ops ensures arithmetic is done with double's for higher accuracy

    static bool last_nack_possibility = true;
    if (max_nacks <= 0) {
        // We can't nack, so just exit. Also takes care of negative case from above calculation.
        if (last_nack_possibility) {
            LOG_INFO("Can't nack anymore! Hit NACK bitrate limit. Try increasing NACK bitrate?");
            last_nack_possibility = false;
        }
        // Nacking has failed when avg_nacks has been saturated.
        // If max_nacks has been saturated, that's just burst bitrate distribution
        return avg_nacks_remaining > 0;
    } else {
        if (!last_nack_possibility) {
            LOG_INFO("NACKing is possible again.");
            last_nack_possibility = true;
        }
    }

    // Track how many nacks we've made this call, to keep it under max_nacks
    int num_packets_nacked = 0;

    // last_missing_frame_nack is strictly increasing so it doesn't need to be throttled
    // Non recovery mode last_packet index is strictly increasing so it doesn't need to be throttled
    // Recovery mode cycles through trying to nack, and we throttle to ~latency,
    // longer during consecutive cycles

    // Nack all the packets we might want to nack about, from oldest to newest,
    // up to max_nacks times
    for (int id = ring_buffer->last_rendered_id + 1;
         id <= ring_buffer->max_id && num_packets_nacked < max_nacks; id++) {
        FrameData* frame_data = get_frame_at_id(ring_buffer, id);
        // If this frame doesn't exist, skip it
        if (frame_data->id != id) {
            // If we've received nothing from a frame before max_id,
            // let's try nacking for index 0 of it
            if (ring_buffer->last_missing_frame_nack < id) {
                // Nack the first set of indices of the missing frame
                // Frames 10-15 packets in size, we'd like to recover in one RTT,
                // But we don't want to try to recover 200 packet frames so quickly
                for (int index = 0; index <= 20; index++) {
                    nack_single_packet(ring_buffer, id, index);
                    num_packets_nacked++;
                }
                LOG_INFO("NACKing for missing Frame ID %d", id);
                ring_buffer->last_missing_frame_nack = id;
            }
            continue;
        }
        // If this frame has been entirely received, there's nothing to nack for
        if (frame_data->packets_received == frame_data->num_packets) {
            continue;
        }

        // =======
        // Go through the frame looking for packets to nack
        // =======

        // Get the last index we received
        int last_packet_received = 0;
        for (int i = frame_data->num_packets - 1; i >= 0; i--) {
            if (frame_data->received_indices[i]) {
                last_packet_received = i;
                break;
            }
        }

        // If too much time has passed since the last packet received,
        // we swap into *recovery mode*, since something is probably wrong with this packet
        if ((id < ring_buffer->max_id ||
             get_timer(frame_data->last_nonnack_packet_timer) > 0.2 * latency) &&
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
            if (get_timer(frame_data->last_nacked_timer) >
                1.2 * latency * frame_data->num_times_nacked) {
#if LOG_NACKING
                LOG_INFO("Attempting to recover Frame ID %d, %d/%d indices received.", id,
                         frame_data->packets_received, frame_data->num_packets);
#endif
                packets_nacked_this_frame = nack_missing_packets_up_to_index(
                    ring_buffer, frame_data, frame_data->num_packets - 1,
                    max_nacks - num_packets_nacked);
                if (frame_data->last_nacked_index == frame_data->num_packets - 1) {
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
    burst_counter += num_packets_nacked;
    avg_counter += num_packets_nacked;

    // Nacking succeeded
    return true;
}

void destroy_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data) {
    /*
        Destroy the frame buffer of frame_data via the block allocator.

        Arguments:
            ring_buffer (RingBuffer*): ring buffer containing the frame we want to destroy
            frame_data (FrameData*): frame data containing the frame we want to destroy

            */
    if (frame_data->frame_buffer != NULL) {
        free_block(ring_buffer->frame_buffer_allocator, frame_data->frame_buffer);
        frame_data->frame_buffer = NULL;
    }
}

void destroy_ring_buffer(RingBuffer* ring_buffer) {
    /*
        Destroy ring_buffer: free all frames and any malloc'ed data, then free the ring buffer

        Arguments:
            ring_buffer (RingBuffer*): ring buffer to destroy
            */
    // free each frame data: in particular, frame_buffer, received_indices, and nacked_indices.
    for (int i = 0; i < ring_buffer->ring_buffer_size; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        destroy_frame_buffer(ring_buffer, frame_data);
        free(frame_data->received_indices);
        free(frame_data->nacked_indices);
    }
    // free received_frames
    free(ring_buffer->receiving_frames);
    // free ring_buffer
    free(ring_buffer);
}
