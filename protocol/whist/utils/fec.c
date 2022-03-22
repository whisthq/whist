//#include <pthread.h>

#include <string.h>
#include <SDL2/SDL_thread.h>

#include "fec.h"
#include "whist/core/whist.h"
#include "rs.h"

// lugi's original library, Vandermonde Maxtrix, O(N^3+ N*X*L) decode
// N is number of original packets, X is num of lost packets, L is max packet length

// other implementations avaliable:
// https://github.com/tahoe-lafs/zfec/  Vandermonde Maxtrix, O(N^3+ N*X*L) decode, modified from
// lugi's, similiar interface, less memory copies and allocations, might be faster, not confirmed

// https://github.com/catid/leopard    FFT, O(NlogN + ???) decode,  buffer_bytes must be a multiple
// of 64
// https://github.com/catid/cm256     Cauchy Matrix, O(N^2+ N*X*L) decode, only works on MSVC
// https://github.com/catid/longhair   Cauchy Matrix, O(N^2+ N*X*L) decode, buffer_bytes must be a
// multiple of 8 time complexity needs futher confirm

// it's not super hard to implement one by ourselves, with O(N^2+ N*X*L) decode, no restriction of
// multiple of 8

/*
Note that this FEC implementation may involve plenty
of unnecessary memcpy's. However, a memcpy at this stage
is really 10x+ faster than the 'memcpy's we're usually worried about,
because this happens on the already encoded frame. Not the decoded frame,
which is much larger. So, we should worry about such optimizations, later.
*/

/*
============================
Defines
============================
*/

#define MAX_NUM_BUFFERS 1024

// The most amount of buffers that RS accepts
#define MAX_RS_BUFFERS 0xff
// We only use 2 bytes to store the buffer size,
// so we need to cap the buffer size as such
#define MAX_BUFFER_SIZE ((1 << (8 * FEC_HEADER_SIZE)) - 1)

#define RS_TABLE_SIZE 256  // size of row and column

// This is the type for the rs_table we use for caching
typedef RSCode* RSTable[RS_TABLE_SIZE][RS_TABLE_SIZE];

struct FECEncoder {
    int num_accepted_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
    int max_packet_size;  // max (original) packet size feed into encoder so far
    RSCode* rs_code;
    bool encode_performed;
};

struct FECDecoder {
    int num_accepted_buffers;
    int num_accepted_real_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
    int max_packet_size;  // max packet size feed into decoder so far
    RSCode* rs_code;
    bool recovery_performed;
};

/*
============================
Globals
============================
*/

// Holds the RSTable for each thread
static SDL_TLSID rs_table_tls_id;

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Gets an rs_code
 *
 * @param k                        The number of original packets
 * @param n                        The total number of packets
 *
 * @returns                        The RSCode for that (n, k) tuple
 */
static RSCode* get_rs_code(int k, int n);

/**
 * @brief                          Frees an RSTable
 *
 * @param opaque                   The RSTable* to free
 */
static void free_rs_code_table(void* opaque);

/*
============================
Public Function Implementations
============================
*/

void init_fec(void) {
    rs_table_tls_id = SDL_TLSCreate();
    init_rs();
}

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
    FATAL_ASSERT(num_real_buffers + num_fec_buffers <= MAX_RS_BUFFERS);
    FATAL_ASSERT(max_buffer_size <= MAX_BUFFER_SIZE);

    FECEncoder* fec_encoder = safe_malloc(sizeof(*fec_encoder));

    fec_encoder->max_buffer_size = max_buffer_size;
    fec_encoder->num_accepted_buffers = 0;
    fec_encoder->num_real_buffers = num_real_buffers;
    int num_total_buffers = num_real_buffers + num_fec_buffers;
    fec_encoder->num_buffers = num_total_buffers;
    fec_encoder->buffers = safe_malloc(sizeof(void*) * num_total_buffers);
    memset(fec_encoder->buffers, 0, sizeof(void*) * num_total_buffers);
    fec_encoder->buffer_sizes = safe_malloc(sizeof(int) * num_total_buffers);
    fec_encoder->max_packet_size = -1;
    fec_encoder->encode_performed = false;

    fec_encoder->rs_code = get_rs_code(num_real_buffers, num_real_buffers + num_fec_buffers);

    return fec_encoder;
}

void fec_encoder_register_buffer(FECEncoder* fec_encoder, void* buffer, int buffer_size) {
    FATAL_ASSERT(buffer != NULL);
    FATAL_ASSERT(0 <= buffer_size);

    char* current_buffer_location = buffer;
    int remaining_buffer_size = buffer_size;
    while (remaining_buffer_size > 0) {
        // If the buffer we were given is larger than max_buffer_size,
        // Then we split it up
        int current_buffer_size = min(remaining_buffer_size, fec_encoder->max_buffer_size);

        // Check that we're not about to accept too many buffers,
        // and then pass the buffer segment into the list of buffer segments
        FATAL_ASSERT(fec_encoder->num_accepted_buffers + 1 <= fec_encoder->num_real_buffers);
        fec_encoder->buffers[fec_encoder->num_accepted_buffers] = current_buffer_location;
        fec_encoder->buffer_sizes[fec_encoder->num_accepted_buffers] = current_buffer_size;
        fec_encoder->max_packet_size = max(fec_encoder->max_packet_size, current_buffer_size);
        fec_encoder->num_accepted_buffers++;

        // Progress the buffer tracker
        current_buffer_location += current_buffer_size;
        remaining_buffer_size -= current_buffer_size;
    }
}

void fec_get_encoded_buffers(FECEncoder* fec_encoder, void** buffers, int* buffer_sizes) {
    FATAL_ASSERT(fec_encoder->num_accepted_buffers == fec_encoder->num_real_buffers);
    if (!fec_encoder->encode_performed) {
        // rs encoder requires packets to have equal length, so we pad packets to max_buffer_size
        for (int i = 0; i < fec_encoder->num_real_buffers; i++) {
            char* original_buffer = fec_encoder->buffers[i];

            FATAL_ASSERT(fec_encoder->buffers[i] != NULL);

            fec_encoder->buffers[i] = safe_malloc(fec_encoder->max_buffer_size);

            memcpy((char*)fec_encoder->buffers[i], original_buffer, fec_encoder->buffer_sizes[i]);
            memset((char*)fec_encoder->buffers[i] + fec_encoder->buffer_sizes[i], 0,
                   fec_encoder->max_buffer_size - fec_encoder->buffer_sizes[i]);
            // TODO, protential optimization
            // if it's the assumption that all packet has same length excpet the last one, and the
            // last one is shorter then only the last one needs padding

            // TODO we can optimize special case of fec_packet num equal to 1  (using XOR)
        }

        // call rs encoder to generate new packets
        for (int i = fec_encoder->num_real_buffers; i < fec_encoder->num_buffers; i++) {
            fec_encoder->buffers[i] = safe_malloc(fec_encoder->max_buffer_size);
            rs_encode(fec_encoder->rs_code, (void**)fec_encoder->buffers, fec_encoder->buffers[i],
                      i, fec_encoder->max_buffer_size);
            fec_encoder->buffer_sizes[i] = fec_encoder->max_buffer_size;
        }

        fec_encoder->encode_performed = true;

    }  // currently we allow fec_get_encoded_buffers to be called multiple times,
    // the code can be simplified a bit if only allow once

    // Populate buffers and buffer_sizes
    for (int i = 0; i < fec_encoder->num_buffers; i++) {
        buffers[i] = fec_encoder->buffers[i];
        buffer_sizes[i] = fec_encoder->buffer_sizes[i];
    }
}

void destroy_fec_encoder(FECEncoder* fec_encoder) {
    int num_total_buffers = fec_encoder->num_buffers;
    if (fec_encoder->encode_performed) {
        for (int i = 0; i < num_total_buffers; i++) {
            if (fec_encoder->buffers[i] != NULL) free(fec_encoder->buffers[i]);
        }
    }
    free(fec_encoder->buffers);
    free(fec_encoder->buffer_sizes);
    free(fec_encoder);
}

FECDecoder* create_fec_decoder(int num_real_buffers, int num_fec_buffers, int max_buffer_size) {
    FATAL_ASSERT(max_buffer_size <= MAX_BUFFER_SIZE);

    FECDecoder* fec_decoder = safe_malloc(sizeof(*fec_decoder));

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
    fec_decoder->num_accepted_real_buffers = 0;
    fec_decoder->max_packet_size = -1;
    fec_decoder->rs_code = get_rs_code(num_real_buffers, num_real_buffers + num_fec_buffers);
    fec_decoder->recovery_performed = false;
    return fec_decoder;
}

void fec_decoder_register_buffer(FECDecoder* fec_decoder, int index, void* buffer,
                                 int buffer_size) {
    FATAL_ASSERT(0 <= index && index < fec_decoder->num_buffers);
    FATAL_ASSERT(0 <= buffer_size && buffer_size <= fec_decoder->max_buffer_size);

    FATAL_ASSERT(fec_decoder->buffer_sizes[index] == -1);

    fec_decoder->buffers[index] = buffer;
    fec_decoder->buffer_sizes[index] = buffer_size;
    fec_decoder->num_accepted_buffers++;
    if (index < fec_decoder->num_real_buffers) {
        fec_decoder->num_accepted_real_buffers++;
    }

    fec_decoder->max_packet_size = max(fec_decoder->max_packet_size, buffer_size);
}

int fec_get_decoded_buffer(FECDecoder* fec_decoder, void* buffer) {
    if (fec_decoder->num_accepted_buffers < fec_decoder->num_real_buffers) {
        return -1;
    }

    bool need_recovery = false;
    // For optimization, we only need recovery, if we need to reconstruct a real packet
    if (fec_decoder->num_accepted_real_buffers != fec_decoder->num_real_buffers) {
        need_recovery = true;
    }

    if (need_recovery && !fec_decoder->recovery_performed) {
        int cnt = 0;
        int* index = safe_malloc(fec_decoder->num_real_buffers * sizeof(int));

        for (int i = 0; i < fec_decoder->num_buffers && cnt < fec_decoder->num_real_buffers; i++) {
            if (fec_decoder->buffer_sizes[i] == -1) continue;
            index[cnt] = i;  // and array required by rs decoder, stores the index of input packets
            FATAL_ASSERT(cnt <= i);
            char* original_buffer = fec_decoder->buffers[i];
            int original_size = fec_decoder->buffer_sizes[i];
            FATAL_ASSERT(fec_decoder->max_packet_size >= original_size);
            fec_decoder->buffers[cnt] = safe_malloc(fec_decoder->max_packet_size);
            memcpy(fec_decoder->buffers[cnt], original_buffer, original_size);
            memset((char*)fec_decoder->buffers[cnt] + original_size, 0,
                   fec_decoder->max_packet_size - original_size);
            // just similiar padding as encoder
            // TODO, protential optimization, similiar to the one in encoder
            cnt++;
        }
        FATAL_ASSERT(cnt == fec_decoder->num_real_buffers);

        // decode
        int res = rs_decode(fec_decoder->rs_code, (void**)fec_decoder->buffers, index,
                            fec_decoder->max_packet_size);
        FATAL_ASSERT(
            res == 0);  // should always success if called correcly,  except malloc fail inside lib
        free(index);
        fec_decoder->recovery_performed = true;

    }  // currently we allow fec_get_decoded_buffer to be called again after succesfully recovered
    // the code can be simplify a bit if not allowing this

    // Write into the provided buffer
    int running_size = 0;
    for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
        int buffer_size = fec_decoder->buffer_sizes[i];
        // If we don't have a buffer_size, use the max_buffer_size
        if (buffer_size == -1) {
            buffer_size = fec_decoder->max_buffer_size;
        }

        if (buffer != NULL) {
            memcpy((char*)buffer + running_size, fec_decoder->buffers[i], buffer_size);
        }
        running_size += buffer_size;
    }

    return running_size;
}

void destroy_fec_decoder(FECDecoder* fec_decoder) {
    if (fec_decoder->recovery_performed) {
        for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
            free(fec_decoder->buffers[i]);
        }
    }
    free(fec_decoder->buffers);
    free(fec_decoder->buffer_sizes);
    free(fec_decoder);
}

/*
============================
Private Function Implementations
============================
*/

static RSCode* get_rs_code(int k, int n) {
    FATAL_ASSERT(k <= n);

    // Get the rs code table for this thread
    RSTable* rs_code_table = SDL_TLSGet(rs_table_tls_id);

    // If the table for this thread doesn't exist, initialize it
    if (rs_code_table == NULL) {
        rs_code_table = (RSTable*)safe_malloc(sizeof(RSTable));
        memset(rs_code_table, 0, sizeof(RSTable));
        SDL_TLSSet(rs_table_tls_id, rs_code_table, free_rs_code_table);
    }

    // If (n, k)'s rs_code hasn't been create yet, create it
    if ((*rs_code_table)[k][n] == NULL) {
        (*rs_code_table)[k][n] = rs_new(k, n);
    }

    // Now return the rs_code for (n, k)
    return (RSCode*)((*rs_code_table)[k][n]);
    // We make a redundant (RSCode*) because cppcheck parses the type wrong
}

static void free_rs_code_table(void* raw_rs_code_table) {
    RSTable* rs_code_table = (RSTable*)raw_rs_code_table;

    // If the table was never created, we have nothing to free
    if (rs_code_table == NULL) return;

    // Find any rs_code entries, and free them
    for (int i = 0; i < RS_TABLE_SIZE; i++) {
        for (int j = i; j < RS_TABLE_SIZE; j++) {
            if ((*rs_code_table)[i][j] != NULL) {
                rs_free((*rs_code_table)[i][j]);
            }
        }
    }

    // Now free the entire table
    free(rs_code_table);
}