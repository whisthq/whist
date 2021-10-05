#include "ringbuffer.h"
#include <lib/bitarray/bitarray.h>
#include <assert.h>

#define MAX_RING_BUFFER_SIZE 500
#define MAX_VIDEO_PACKETS 500
#define MAX_AUDIO_PACKETS 3
#define MAX_UNORDERED_PACKETS 10

void reset_ring_buffer(RingBuffer* ring_buffer);
void nack_missing_frames(RingBuffer* ring_buffer, int start_id, int end_id);
void nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int index);
void init_frame(RingBuffer* ring_buffer, int id, int num_indices);
void allocate_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);
void destroy_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);

struct bit_array_t {
    unsigned char* array; /* pointer to array containing bits */
    unsigned int numBits; /* number of bits in array */
};

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
    ring_buffer->last_received_nonnack_id = -1;
    ring_buffer->max_id = -1;
    ring_buffer->frames_received = 0;
    start_timer(&ring_buffer->missing_frame_nack_timer);
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

RingBuffer* init_ring_buffer(FrameDataType type, int ring_buffer_size) {
    /*
        Initialize the ring buffer; malloc space for all the frames and set their IDs to -1.

        Arguments:
            type (FrameDataType): audio or video
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
    if (!ring_buffer->receiving_frames) {
        return NULL;
    }
    ring_buffer->largest_num_packets =
        ring_buffer->type == FRAME_VIDEO ? MAX_VIDEO_PACKETS : MAX_AUDIO_PACKETS;
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
        ring_buffer->type == FRAME_VIDEO ? LARGEST_VIDEOFRAME_SIZE : LARGEST_AUDIOFRAME_SIZE;

    ring_buffer->frame_buffer_allocator = create_block_allocator(ring_buffer->largest_frame_size);
    ring_buffer->currently_rendering_id = -1;

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
    // frame_data->rendered = false;
    frame_data->frame_size = 0;
    frame_data->type = ring_buffer->type;
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
    frame_data->packets_received = 0;
    frame_data->num_packets = 0;
    frame_data->last_nacked_index = -1;
    frame_data->num_times_nacked = 0;
    // frame_data->rendered = false;
    frame_data->frame_size = 0;
}

void set_rendering(RingBuffer* ring_buffer, int id) {
    /*
        Indicate that the frame with ID id is currently rendering and free the frame buffer for the
        previously rendering frame. Ownership of the frame buffer for the rendering frame is
        transferred to ring_buffer->currently_rendering_frame, allowing us to fully wipe the ring
        buffer's receiving_frames array if we fall too behind.

        Arguments:
            ring_buffer (RingBuffer*): ring buffer metadata to change
            id (int): ID of the frame we are currently rendering.
    */
    if (ring_buffer->currently_rendering_id != -1) {
        // destroy the frame buffer of the currently rendering frame
        destroy_frame_buffer(ring_buffer, &ring_buffer->currently_rendering_frame);
    }
    // set the currently rendering ID and frame
    ring_buffer->currently_rendering_id = id;
    FrameData* current_frame = get_frame_at_id(ring_buffer, id);
    ring_buffer->currently_rendering_frame = *current_frame;
    // clear the current frame's data
    current_frame->frame_buffer = NULL;
    reset_frame(ring_buffer, current_frame);
}

int receive_packet(RingBuffer* ring_buffer, FractalPacket* packet) {
    /*
        Process a FractalPacket and add it to the ring buffer. If the packet belongs to an existing
        frame, copy its data into the frame; if it belongs to a new frame, initialize the frame and
        copy data. Nack for missing packets (of the packet's frame) and missing frames (before the
        current frame).

        Arguments:
            ring_buffer (RingBuffer*): ring buffer to place the packet in
            packet (FractalPacket*): UDP packet for either audio or video

        Returns:
            (int): 1 if we overwrote a valid frame, 0 on success, -1 on failure
            */
    FrameData* frame_data = get_frame_at_id(ring_buffer, packet->id);

    ring_buffer->num_packets_received++;

    bool overwrote_frame = false;
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
            overwrote_frame = true;
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
                reset_ring_buffer(ring_buffer);
            }
        }
        // Now, we can overwrite with no other concerns
        init_frame(ring_buffer, packet->id, packet->num_indices);
    }

    start_timer(&frame_data->last_packet_timer);
    // check if we have nacked for the packet
    // TODO: log video vs audio
    if (packet->is_a_nack) {
        if (!frame_data->received_indices[packet->index]) {
            LOG_INFO("NACK for ID %d, Index %d received!", packet->id, packet->index);
        } else {
            LOG_INFO("NACK for ID %d, Index %d received, but didn't need it.", packet->id,
                     packet->index);
        }
    } else if (frame_data->nacked_indices[packet->index] > 0) {
        LOG_INFO("Received original ID %d, Index %d, but we had NACK'ed for it.", packet->id,
                 packet->index);
    }

    // If we have already received the packet, there is nothing to do
    if (frame_data->received_indices[packet->index]) {
        LOG_INFO("Duplicate of ID %d, Index %d received", packet->id, packet->index);
        return -1;
    }

    // Update framedata metadata + buffer
    ring_buffer->max_id = max(ring_buffer->max_id, frame_data->id);
    frame_data->received_indices[packet->index] = true;
    if (ring_buffer->last_received_nonnack_id != -1 && !packet->is_a_nack) {
        // If we receive a normal packet, but that packet has an ID that's significantly
        // beyond the last received normal packet's ID, then we probably need to nack
        // for the frames that are in the middle
        nack_missing_frames(ring_buffer, ring_buffer->last_received_nonnack_id + 1, frame_data->id);
    }
    // because UDP packets can arrive out of order, we should nack only up to MAX_UNORDERED_PACKETS
    nack_missing_packets_up_to_index(ring_buffer, frame_data,
                                     packet->index - MAX_UNORDERED_PACKETS);
    if (!packet->is_a_nack) {
        ring_buffer->last_received_nonnack_id = frame_data->id;
    }
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
    if (overwrote_frame) {
        return 1;
    } else {
        return 0;
    }
}

void nack_single_packet(RingBuffer* ring_buffer, int id, int index) {
    /*
        Nack the packet at ID id and index index.

        Arguments:
            ring_buffer (RingBuffer*): the ring buffer waiting for the packet
            id (int): Frame ID of the packet
            index (int): index of the packet
            */
    ring_buffer->num_packets_nacked++;
    LOG_INFO("NACKing for Packet ID %d, Index %d", id, index);
    FractalClientMessage fmsg = {0};
    fmsg.type = ring_buffer->type == FRAME_AUDIO ? MESSAGE_AUDIO_NACK : MESSAGE_VIDEO_NACK;

    fmsg.simple_nack.id = id;
    fmsg.simple_nack.index = index;
    send_fmsg(&fmsg);
}

void nack_bitarray_packets(RingBuffer* ring_buffer, int id, int start_index, bit_array_t* bit_arr) {
    /*
        Nack the packets at ID id and start_index start_index.

        Arguments:
            ring_buffer (RingBuffer*): the ring buffer waiting for the packet
            id (int): Frame ID of the packet
            start_index (int): index of the first packet to be NACKed
            bit_arr (bit_array_t): the bit array with the indexes of the packets to NACK.
            */

    LOG_INFO("entering nack_bitarray_packets, id:%i, start_index:%i", id, start_index);

    FractalClientMessage fmsg = {0};
    if (ring_buffer->type == FRAME_AUDIO) {
        fmsg.type = MESSAGE_AUDIO_BITARRAY_NACK;
        fmsg.bitarray_audio_nack.id = id;
        fmsg.bitarray_audio_nack.index = start_index;
        memset(fmsg.bitarray_audio_nack.ba_raw, 0, BITS_TO_CHARS(bit_arr->numBits));
        memcpy(fmsg.bitarray_audio_nack.ba_raw, BitArrayGetBits(bit_arr),
               BITS_TO_CHARS(bit_arr->numBits));
        fmsg.bitarray_audio_nack.numBits = bit_arr->numBits;
    } else {
        fmsg.type = MESSAGE_VIDEO_BITARRAY_NACK;
        fmsg.bitarray_video_nack.id = id;
        fmsg.bitarray_video_nack.index = start_index;
        memset(fmsg.bitarray_audio_nack.ba_raw, 0, BITS_TO_CHARS(bit_arr->numBits));
        memcpy(fmsg.bitarray_video_nack.ba_raw, BitArrayGetBits(bit_arr),
               BITS_TO_CHARS(bit_arr->numBits));
        fmsg.bitarray_video_nack.numBits = bit_arr->numBits;
    }

    for (unsigned int i = 0; i < bit_arr->numBits; i++) {
        if (BitArrayTestBit(bit_arr, i)) {
            ring_buffer->num_packets_nacked++;
            LOG_INFO("Bitarray-NACKing for Packet ID %d, Index %d", id, start_index + i);
        }
    }

    send_fmsg(&fmsg);
}

void nack_missing_frames(RingBuffer* ring_buffer, int start_id, int end_id) {
    /*
        Nack missing frames between start_id (inclusive) and end_id (exclusive).

        Arguments:
            ring_buffer (RingBuffer*): ring buffer missing the frames
            start_id (int): First frame to check
            end_id (int): Last frame + 1 to check

        */
    if (get_timer(ring_buffer->missing_frame_nack_timer) > 25.0 / 1000) {
        // We min to ensure that we don't nack more than 3 times for something like this
        for (int i = start_id; i < min(start_id + 3, end_id); i++) {
            if (get_frame_at_id(ring_buffer, i)->id != i) {
                LOG_INFO("Missing all packets for frame %d, NACKing now for index 0", i);
                start_timer(&ring_buffer->missing_frame_nack_timer);
                nack_single_packet(ring_buffer, i, 0);
            }
        }
    }
}

void nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int index) {
    /*
        Nack up to 1 missing packet up to index

        Arguments:
            ring_buffer (RingBuffer*): ring buffer missing packets
            frame_data (FrameData*): frame missing packets
            index (int): index up to which to nack missing packets
    */

    if (index > 0 && get_timer(frame_data->last_nacked_timer) > 6.0 / 1000) {
        // LOG_INFO("nack_missing_packets_up_to_index with index=%i", index);

        int first_packet_to_nack = -1;
        // Create the bitmap handle, we don't know the size yet
        bit_array_t* bit_arr = NULL;
        bool simple_nacking = true;

        for (int i = max(0, frame_data->last_nacked_index + 1); i <= index; i++) {
            if (!frame_data->received_indices[i]) {
                if (!bit_arr && frame_data->nacked_indices[i] < MAX_PACKET_NACKS) {
                    assert(first_packet_to_nack == -1);
                    // Found first packet to NACK!
                    bit_arr = BitArrayCreate(index - i + 1);
                    LOG_INFO("nack_missing_packets_up_to_index with index=%i", index);
                    LOG_INFO("        Creating bit_arr of size %i", index - i + 1);
                    BitArrayClearAll(bit_arr);
                    first_packet_to_nack = i;
                    BitArraySetBit(bit_arr, i);
                } else if (bit_arr) {
                    assert(first_packet_to_nack != -1);
                    // We already have a candidate packet for NACKing. Check if we can NACK for more
                    // using the bitarray.
                    if (frame_data->nacked_indices[i] < MAX_PACKET_NACKS) {
                        BitArraySetBit(bit_arr, i);
                        simple_nacking = false;
                    } else {
                        break;
                    }
                }
            }
        }

        if (simple_nacking) {
            if (bit_arr) {
                LOG_INFO(
                    "\tbit_arr is non-NULL, and we are doing simple nacking, so we can destroy it "
                    "now!");
                BitArrayDestroy(bit_arr);
            }
            if (first_packet_to_nack != -1) {
                LOG_INFO("\t Simple nacking packet from frame:%i and with id:%i", frame_data->id,
                         first_packet_to_nack);
                frame_data->last_nacked_index =
                    max(frame_data->last_nacked_index, first_packet_to_nack);
                frame_data->nacked_indices[first_packet_to_nack]++;
                nack_single_packet(ring_buffer, frame_data->id, first_packet_to_nack);

                start_timer(&frame_data->last_nacked_timer);
            }  // else {
               // LOG_INFO("\t first_packet_to_nack==-1!");
            //}
        } else {
            LOG_INFO("\t Nacking using bitarray!");
            for (int i = 0; i < (int)bit_arr->numBits; i++) {
                if (BitArrayTestBit(bit_arr, i)) {
                    frame_data->nacked_indices[first_packet_to_nack + i]++;
                    frame_data->last_nacked_index =
                        max(frame_data->last_nacked_index, first_packet_to_nack + i);
                }
            }
            nack_bitarray_packets(ring_buffer, frame_data->id, first_packet_to_nack, bit_arr);
            LOG_INFO("Done nacking packets, destroying BitArray now!");
            BitArrayDestroy(bit_arr);
            start_timer(&frame_data->last_nacked_timer);
        }
    }
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
