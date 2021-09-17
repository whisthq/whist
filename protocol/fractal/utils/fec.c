#include "fec.h"
#include "fractal/core/fractal.h"

/*
Note that this FEC implementation may involve plenty
of unnecessary memcpy's. However, a memcpy at this stage
is really 10x+ faster than the 'memcpy's we're usually worried about,
because this happens on the already encoded frame. Not the decoded frame,
which is much larger. So, we should worry about such optimizations, later.
*/

#define MAX_NUM_BUFFERS 1024
#define M 20

typedef struct {
    int num_buffers;
    int num_real_buffers;
    int* buffer_sizes;
    char** buffers;
} FecInternalContext;

fec_context* create_fec_encoder_context() {
    FecInternalContext* fec_encoder = safe_malloc(sizeof(FecInternalContext));
    fec_encoder->num_buffers = 0;
    fec_encoder->buffers = safe_malloc(sizeof(char*) * MAX_NUM_BUFFERS);
    fec_encoder->buffer_sizes = safe_malloc(sizeof(int*) * MAX_NUM_BUFFERS);

    return fec_encoder;
}

fec_context* create_fec_decoder_context(int num_real_buffers, int num_total_buffers) {
    FecInternalContext* fec_decoder = safe_malloc(sizeof(FecInternalContext));

    fec_decoder->num_real_buffers = num_real_buffers;
    fec_decoder->num_buffers = num_total_buffers;
    fec_decoder->buffers = safe_malloc(sizeof(char*) * num_total_buffers);
    fec_decoder->buffer_sizes = safe_malloc(sizeof(int*) * num_total_buffers);
    for (int i = 0; i < num_total_buffers; i++) {
        fec_decoder->buffer_sizes[i] = -1;
    }

    return fec_decoder;
}

void free_fec_context(fec_context* fec_context_raw) {
    FecInternalContext* fec_context = (FecInternalContext*)fec_context_raw;
    free(fec_context->buffers);
    free(fec_context->buffer_sizes);
    free(fec_context);
}

void encoder_register_buffer(fec_context* fec_encoder_raw, char* buffer, int buffer_size) {
    FecInternalContext* fec_encoder = (FecInternalContext*)fec_encoder_raw;
    if (fec_encoder->num_buffers == MAX_NUM_BUFFERS) {
        LOG_ERROR("Tried to register too many buffers!");
        return;
    }
    fec_encoder->buffers[fec_encoder->num_buffers] = buffer;
    fec_encoder->buffer_sizes[fec_encoder->num_buffers] = buffer_size;
    fec_encoder->num_buffers++;
}

void decoder_register_buffer(fec_context* fec_decoder_raw, int index, char* buffer,
                             int buffer_size) {
    FecInternalContext* fec_decoder = (FecInternalContext*)fec_decoder_raw;
    if (index < 0 || index >= fec_decoder->num_buffers) {
        LOG_FATAL("Index %d out of bounds! Size of %d", index, fec_decoder->num_buffers);
    }
    fec_decoder->buffers[index] = buffer;
    fec_decoder->buffer_sizes[index] = buffer_size;
}

void get_encoded_buffers(fec_context* fec_encoder_raw, char** buffers, int** buffer_sizes,
                         int* num_buffers) {
    FecInternalContext* fec_encoder = (FecInternalContext*)fec_encoder_raw;

    *num_buffers = fec_encoder->num_buffers + M;
    if (*num_buffers >= MAX_NUM_BUFFERS) {
        LOG_FATAL("Cannot fit %d FEC buffers into MAX_NUM_BUFFERS, please increase MAX_NUM_BUFFERS",
                  *num_buffers);
    }

    // Just wrap around for the extra M buffers
    for (int i = 0; i < *num_buffers; i++) {
        fec_encoder->buffers[i] = fec_encoder->buffers[i % fec_encoder->num_buffers];
        fec_encoder->buffer_sizes[i] = fec_encoder->buffer_sizes[i % fec_encoder->num_buffers];
    }
}

int get_decoded_buffer(fec_context* fec_decoder_raw, char* buffer, int* buffer_size) {
    FecInternalContext* fec_decoder = (FecInternalContext*)fec_decoder_raw;

    for (int i = 0; i < fec_decoder->num_buffers; i++) {
        // If a missing packet needs to be repaired using a redundant packet, do so
        if (fec_decoder->buffer_sizes[i % fec_decoder->num_real_buffers] == -1 &&
            fec_decoder->buffer_sizes[i] != -1) {
            fec_decoder->buffers[i % fec_decoder->num_real_buffers] = fec_decoder->buffers[i];
            fec_decoder->buffer_sizes[i % fec_decoder->num_real_buffers] =
                fec_decoder->buffer_sizes[i];
        }
    }

    // Write into the provided buffer
    int running_size = 0;
    for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
        if (fec_decoder->buffer_sizes[i] == -1) {
            LOG_WARNING("Could not reconstruct original message, too many erasures!");
            return -1;
        }
        if (running_size + fec_decoder->buffer_sizes[i] >= *buffer_size) {
            LOG_ERROR("Buffer for FEC data is too large! Overflowing buffer of size %d",
                      *buffer_size);
            return -1;
        }
        memcpy(buffer + running_size, fec_decoder->buffers[i], fec_decoder->buffer_sizes[i]);
        running_size += fec_decoder->buffer_sizes[i];
    }

    // Let the caller know the size of the real buffer
    *buffer_size = running_size;

    return 0;
}
