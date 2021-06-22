#include "ringbuffer.h"

// TODO: figure out a good max size
#define MAX_RING_BUFFER_SIZE 500
#define LARGEST_AUDIO_FRAME_SIZE 9000
#define LARGEST_VIDEO_FRAME_SIZE 1000000

void nack_missing_frames(RingBuffer* ring_buffer, int start_id, int end_id);
void nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int index);
void init_frame(RingBuffer* ring_buffer, int id, int num_indices);
void allocate_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);
void destroy_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data);

RingBuffer* init_ring_buffer(FrameDataType type, int ring_buffer_size) {
    if (ring_buffer_size > MAX_RING_BUFFER_SIZE) {
        LOG_ERROR("Requested ring buffer size %d too large - ensure size is at most %d", ring_buffer_size, MAX_RING_BUFFER_SIZE);
        return NULL;
    }
    RingBuffer *ring_buffer = safe_malloc(sizeof(RingBuffer));
    if (!ring_buffer) {
        return NULL;
    }

    ring_buffer->type = type;
    ring_buffer->ring_buffer_size = ring_buffer_size;
    ring_buffer->receiving_frames = safe_malloc(ring_buffer_size * sizeof(FrameData));
    if (!ring_buffer->receiving_frames) {
        return NULL;
    }
    for (int i = 0; i < ring_buffer_size; i++) {
        ring_buffer->receiving_frames[i].id = -1;
    }
    ring_buffer->last_received_id = -1;
    ring_buffer->max_id = -1;
    ring_buffer->num_nacked = 0;

    ring_buffer->largest_frame_size = ring_buffer->type == FRAME_VIDEO ? LARGEST_VIDEO_FRAME_SIZE : LARGEST_AUDIO_FRAME_SIZE;

    if (ring_buffer->type == FRAME_VIDEO) {
        ring_buffer->frame_buffer_allocator = create_block_allocator(ring_buffer->largest_frame_size);
    } else {
        ring_buffer->frame_buffer_allocator = NULL;
    }

    start_timer(&ring_buffer->missing_frame_nack_timer);
    return ring_buffer;
}


FrameData* get_frame_at_id(RingBuffer* ring_buffer, int id) {
    // TODO: should there be checks to make sure IDs agree?
    return &ring_buffer->receiving_frames[id % ring_buffer->ring_buffer_size];
}

void allocate_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data) {
    if (frame_data->frame_buffer == NULL) {
        if (ring_buffer->type == FRAME_AUDIO) {
            frame_data->frame_buffer = safe_malloc(ring_buffer->largest_frame_size);
        } else {
            frame_data->frame_buffer = allocate_block(ring_buffer->frame_buffer_allocator);
        }
    }
}

void init_frame(RingBuffer* ring_buffer, int id, int num_indices) {
    FrameData* frame_data = get_frame_at_id(ring_buffer, id);
        // initialize new framedata
        // TODO: figure out how to check whether or not we are rendering
        frame_data->id = id;
        allocate_frame_buffer(ring_buffer, frame_data);
        frame_data->packets_received = 0;
        frame_data->num_packets = num_indices;
        // allocate the boolean arrays
        int indices_array_size = frame_data->num_packets * sizeof(bool);
        frame_data->received_indices = safe_malloc(indices_array_size);
        frame_data->nacked_indices = safe_malloc(indices_array_size);
        memset(frame_data->received_indices, 0, indices_array_size);
        memset(frame_data->nacked_indices, 0, indices_array_size);
        frame_data->last_nacked_index = -1;
        frame_data->num_times_nacked = -1;
        frame_data->rendered = false;
        frame_data->frame_size = 0;
        frame_data->type = ring_buffer->type;
        start_timer(&frame_data->frame_creation_timer);
        start_timer(&frame_data->last_nacked_timer);
}

int receive_packet(RingBuffer* ring_buffer, FractalPacket* packet) {
    FrameData* frame_data = get_frame_at_id(ring_buffer, packet->id);
    if (packet->id < frame_data->id) {
        LOG_INFO("Old packet (ID %d) received, previous ID %d", packet->id, frame_data->id);
        return -1;
    } else if (packet->id > frame_data->id) {
        init_frame(ring_buffer, packet->id, packet->num_indices);
    }

    start_timer(&frame_data->last_packet_timer);
    // check if we have nacked for the packet
    // TODO: log video vs audio
    if (packet->is_a_nack) {
        if (!frame_data->received_indices[packet->index]) {
            LOG_INFO("NACK for ID %d, Index %d received!", packet->id, packet->index);
        } else {
            LOG_INFO("NACK for ID %d, Index %d received, but didn't need it.", packet->id, packet->index);
        }
    } else if (frame_data->nacked_indices[packet->index]) {
        LOG_INFO("Received original ID %d, Index %d, but we had NACK'ed for it.", packet->id, packet->index);
    }

    // If we have already received the packet, there is nothing to do
    if (frame_data->received_indices[packet->index]) {
        LOG_INFO("Duplicate of ID %d, Index %d received", packet->id, packet->index);
        return 0;
    }

    // Update framedata metadata + buffer
    ring_buffer->max_id = max(ring_buffer->max_id, frame_data->id);
    frame_data->received_indices[packet->index] = true;
    // TODO: NACK
    nack_missing_frames(ring_buffer, ring_buffer->last_received_id + 1, frame_data->id);
    nack_missing_packets_up_to_index(ring_buffer, frame_data, packet->index);
    ring_buffer->last_received_id = frame_data->id;
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
    return 0;
}

void nack_packet(RingBuffer* ring_buffer, int id, int index) {
    ring_buffer->num_nacked++;
    LOG_INFO("NACKing for Packet ID %d, Index %d", id, index);
    FractalClientMessage fmsg = {0};
    fmsg.type = ring_buffer->type == FRAME_AUDIO ? MESSAGE_AUDIO_NACK : MESSAGE_VIDEO_NACK;
    fmsg.nack_data.id = id;
    fmsg.nack_data.index = index;
    send_fmsg(&fmsg);
}

void nack_missing_frames(RingBuffer* ring_buffer, int start_id, int end_id) {
    if (get_timer(ring_buffer->missing_frame_nack_timer) > 25.0 / 1000) {
        for (int i = start_id; i < end_id; i++) {
            if (get_frame_at_id(ring_buffer, i)->id != i) {
                LOG_INFO("Missing all packets for frame %d, NACKing now for index 0", i);
                start_timer(&ring_buffer->missing_frame_nack_timer);
                nack_packet(ring_buffer, i, 0);
            }
        }
    }
}

void nack_missing_packets_up_to_index(RingBuffer* ring_buffer, FrameData* frame_data, int index) {
    if (index > 0 && get_timer(frame_data->last_nacked_timer) > 6.0 / 1000) {
        int to_index = index - 5;
        for (int i = max(0, frame_data->last_nacked_index + 1); i <= to_index; i++) {
            frame_data->last_nacked_index = max(frame_data->last_nacked_index, i);
            if (!frame_data->received_indices[i]) {
                frame_data->nacked_indices[i] = true;
            }
            nack_packet(ring_buffer, frame_data->id, i);
            start_timer(&frame_data->last_nacked_timer);
            break;
        }
    }
}

void destroy_frame_buffer(RingBuffer* ring_buffer, FrameData* frame_data) {
    if (frame_data->frame_buffer != NULL) {
        if (ring_buffer->type == FRAME_AUDIO) {
            free(frame_data->frame_buffer);
        } else {
            free_block(ring_buffer->frame_buffer_allocator, frame_data->frame_buffer);
        }
    }
}

void destroy_ring_buffer(RingBuffer* ring_buffer) {
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
