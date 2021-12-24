#include "fec.h"
#include "whist/core/whist.h"
/*
Note that this FEC implementation may involve plenty
of unnecessary memcpy's. However, a memcpy at this stage
is really 10x+ faster than the 'memcpy's we're usually worried about,
because this happens on the already encoded frame. Not the decoded frame,
which is much larger. So, we should worry about such optimizations, later.
*/

#define MAX_NUM_BUFFERS 1024

typedef struct {
    int num_accepted_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
} InternalFECEncoder;

typedef struct {
    int num_accepted_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
} InternalFECDecoder;

// num_fec_packets / (num_fec_packets + num_indices) = context->fec_packet_ratio
// a / (a + b) = c
// a = ac + bc
// a(1-c) = bc
// a = bc/(1-c)
int get_num_fec_packets(int num_real_packets, double fec_packet_ratio) {
    double ratio = fec_packet_ratio / (1.0 - fec_packet_ratio);
    return num_real_packets * ratio;
}

FECEncoder* create_fec_encoder(int num_real_buffers, int num_fec_buffers, int max_buffer_size) {
    InternalFECEncoder* fec_encoder = safe_malloc(sizeof(InternalFECEncoder));

    fec_encoder->max_buffer_size = max_buffer_size;
    fec_encoder->num_accepted_buffers = 0;
    fec_encoder->num_real_buffers = num_real_buffers;
    int num_total_buffers = num_real_buffers + num_fec_buffers;
    fec_encoder->num_buffers = num_total_buffers;
    fec_encoder->buffers = safe_malloc(sizeof(void*) * num_total_buffers);
    fec_encoder->buffer_sizes = safe_malloc(sizeof(int) * num_total_buffers);

    return fec_encoder;
}

void fec_encoder_register_buffer(FECEncoder* fec_encoder_raw, void* buffer, int buffer_size) {
    InternalFECEncoder* fec_encoder = (InternalFECEncoder*)fec_encoder_raw;

    FATAL_ASSERT(fec_encoder->num_accepted_buffers < fec_encoder->num_real_buffers);
    FATAL_ASSERT(0 <= buffer_size && buffer_size <= fec_encoder->max_buffer_size);

    fec_encoder->buffers[fec_encoder->num_accepted_buffers] = buffer;
    fec_encoder->buffer_sizes[fec_encoder->num_accepted_buffers] = buffer_size;
    fec_encoder->num_accepted_buffers++;
}

void fec_get_encoded_buffers(FECEncoder* fec_encoder_raw, void** buffers, int* buffer_sizes) {
    InternalFECEncoder* fec_encoder = (InternalFECEncoder*)fec_encoder_raw;

    FATAL_ASSERT(fec_encoder->num_accepted_buffers == fec_encoder->num_real_buffers);

    // Just wrap around to generate the extra FEC buffers
    for (int i = fec_encoder->num_real_buffers; i < fec_encoder->num_buffers; i++) {
        fec_encoder->buffers[i] = fec_encoder->buffers[i % fec_encoder->num_real_buffers];
        fec_encoder->buffer_sizes[i] = fec_encoder->buffer_sizes[i % fec_encoder->num_real_buffers];
    }

    // Populate buffers and buffer_sizes
    for (int i = 0; i < fec_encoder->num_buffers; i++) {
        buffers[i] = fec_encoder->buffers[i];
        buffer_sizes[i] = fec_encoder->buffer_sizes[i];
    }
}

void destroy_fec_encoder(FECEncoder* fec_context_raw) {
    InternalFECEncoder* fec_context = (InternalFECEncoder*)fec_context_raw;

    free(fec_context->buffers);
    free(fec_context->buffer_sizes);
    free(fec_context);
}

FECDecoder* create_fec_decoder(int num_real_buffers, int num_fec_buffers, int max_buffer_size) {
    InternalFECDecoder* fec_decoder = safe_malloc(sizeof(InternalFECDecoder));

    int num_total_buffers = num_real_buffers + num_fec_buffers;

    fec_decoder->max_buffer_size = max_buffer_size;
    fec_decoder->num_real_buffers = num_real_buffers;
    fec_decoder->num_buffers = num_total_buffers;
    fec_decoder->buffers = safe_malloc(sizeof(void*) * num_total_buffers);
    fec_decoder->buffer_sizes = safe_malloc(sizeof(int*) * num_total_buffers);
    for (int i = 0; i < num_total_buffers; i++) {
        fec_decoder->buffer_sizes[i] = -1;
    }
    fec_decoder->num_accepted_buffers = 0;
    return fec_decoder;
}

void fec_decoder_register_buffer(FECDecoder* fec_decoder_raw, int index, void* buffer,
                                 int buffer_size) {
    InternalFECDecoder* fec_decoder = (InternalFECDecoder*)fec_decoder_raw;

    FATAL_ASSERT(0 <= index && index < fec_decoder->num_buffers);
    FATAL_ASSERT(0 <= buffer_size && buffer_size <= fec_decoder->max_buffer_size);

    fec_decoder->buffers[index] = buffer;
    fec_decoder->buffer_sizes[index] = buffer_size;
    fec_decoder->num_accepted_buffers++;
}

int fec_get_decoded_buffer(FECDecoder* fec_decoder_raw, void* buffer) {
    InternalFECDecoder* fec_decoder = (InternalFECDecoder*)fec_decoder_raw;

    // Loop over the redundant packets
    for (int i = fec_decoder->num_real_buffers; i < fec_decoder->num_buffers; i++) {
        // If a missing packet needs to be repaired using a redundant packet, try to do so
        int origin_idx = i % fec_decoder->num_real_buffers;
        if (fec_decoder->buffer_sizes[origin_idx] == -1 && fec_decoder->buffer_sizes[i] != -1) {
            fec_decoder->buffers[origin_idx] = fec_decoder->buffers[i];
            fec_decoder->buffer_sizes[origin_idx] = fec_decoder->buffer_sizes[i];
        }
    }

    // Return false if it wasn't possible to reconstruct the message
    for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
        if (fec_decoder->buffer_sizes[i] == -1) {
            return -1;
        }
    }

    // Write into the provided buffer
    int running_size = 0;
    for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
        // Don't actually write, if the caller doesn't want the data
        if (buffer != NULL) {
            memcpy((char*)buffer + running_size, fec_decoder->buffers[i],
                   fec_decoder->buffer_sizes[i]);
        }
        running_size += fec_decoder->buffer_sizes[i];
    }

    // Return the size of the decoded buffer.
    return running_size;
}

void destroy_fec_decoder(FECDecoder* fec_context_raw) {
    InternalFECDecoder* fec_context = (InternalFECDecoder*)fec_context_raw;

    free(fec_context->buffers);
    free(fec_context->buffer_sizes);
    free(fec_context);
}
