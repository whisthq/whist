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

#define MAX_NUM_BUFFERS 1024

const int max_u8 = 0xff;
const int max_u16 = 0xffff;
#define RS_TABLE_SIZE 256  // size of row and column

static SDL_SpinLock tls_lock;      // the spin lock used for SDL_TLSCreate() and SDL_CreateMutex()
static SDL_TLSID tls_id;           // the key for thread-specific data
static SDL_mutex* fec_init_mutex;  // switch to this mutex after it's initilized

bool fec_initalized = false;

typedef void* (*rs_table_t)[RS_TABLE_SIZE];  // NOLINT  //supress clang-tidy warning
// a thread-specific table to cache RS coder, for efficiency,  avoid create and destroy again and
// again

void free_rs_code_table(
    void* dummy_ptr)  // destructor for thread-specific data, clean the table after thread exit
{
    LOG_INFO("free_rs_code_table() called");
    rs_table_t rs_code_table;
    rs_code_table = SDL_TLSGet(tls_id);
    if (rs_code_table == NULL) return;
    for (int i = 0; i < RS_TABLE_SIZE; i++) {
        for (int j = i; j < RS_TABLE_SIZE; j++) {
            if (rs_code_table[i][j]) {
                fec_free(rs_code_table[i][j]);
            }
        }
    }
    free(rs_code_table);
}

// get RS coder for the specific setting, cache RS coder in the table
// note in the RS lib, k means num of original packets, n means total packets
void* get_rs_code(int k, int n) {
    if (!tls_id) {
        SDL_AtomicLock(&tls_lock);  // SDL_CreateMutex() and SDL_TLSCreate() should be called only
                                    // once, use spin lock for protection
        if (!tls_id) {
            fec_init_mutex = SDL_CreateMutex();
            tls_id = SDL_TLSCreate();
            // see https://wiki.libsdl.org/SDL_TLSCreate for the offical suggested pattern for
            // SDL_TLSCreate
        }
        SDL_AtomicUnlock(&tls_lock);
    }

    rs_table_t rs_code_table = SDL_TLSGet(tls_id);  // get thread-specific data, i.e. the table

    if (rs_code_table == NULL) {
        SDL_LockMutex(fec_init_mutex);  // init_fec() should be only called once, required by RS
                                        // lib.
        // it's more costly than SDL_CreateMutex() and SDL_TLSCreate(), so we switch to mutex,
        // instead of spinlock
        if (fec_initalized == false) {
            init_fec();
            fec_initalized = true;
        }
        SDL_UnlockMutex(fec_init_mutex);

        rs_code_table = (rs_table_t)safe_malloc(sizeof(void*) * RS_TABLE_SIZE * RS_TABLE_SIZE);
        memset(rs_code_table, 0, sizeof(void*) * RS_TABLE_SIZE * RS_TABLE_SIZE);
        SDL_TLSSet(tls_id, rs_code_table,
                   free_rs_code_table);  // set thread-specific data, i.e. the table
    }

    if (rs_code_table[k][n] == NULL) {
        rs_code_table[k][n] = fec_new(k, n);
    }
    return (void*)rs_code_table[k][n];
}

typedef struct {
    int num_accepted_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
    int max_packet_size;  // max (original) packet size feed into encoder so far
    void* rs_code;
    bool encode_performed;
} InternalFECEncoder;

typedef struct {
    int num_accepted_buffers;
    int num_accepted_real_buffers;
    int num_buffers;
    int num_real_buffers;
    int max_buffer_size;
    int* buffer_sizes;
    void** buffers;
    int max_packet_size;  // max packet size feed into decoder so far
    void* rs_code;
    bool recovery_performed;
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

    FATAL_ASSERT(num_real_buffers + num_fec_buffers <= max_u8);

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

void fec_encoder_register_buffer(FECEncoder* fec_encoder_raw, void* buffer, int buffer_size) {
    InternalFECEncoder* fec_encoder = (InternalFECEncoder*)fec_encoder_raw;

    FATAL_ASSERT(fec_encoder->num_accepted_buffers < fec_encoder->num_real_buffers);
    FATAL_ASSERT(0 <= buffer_size && buffer_size <= fec_encoder->max_buffer_size);

    FATAL_ASSERT(buffer_size <= max_u16);

    fec_encoder->buffers[fec_encoder->num_accepted_buffers] = buffer;
    fec_encoder->buffer_sizes[fec_encoder->num_accepted_buffers] = buffer_size;
    fec_encoder->max_packet_size = max(fec_encoder->max_packet_size, buffer_size);
    fec_encoder->num_accepted_buffers++;
}

void fec_get_encoded_buffers(FECEncoder* fec_encoder_raw, void** buffers, int* buffer_sizes) {
    InternalFECEncoder* fec_encoder = (InternalFECEncoder*)fec_encoder_raw;

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
            fec_encoder->buffers[i] = safe_malloc(fec_encoder->max_packet_size);
            fec_encode(fec_encoder->rs_code, (void**)fec_encoder->buffers, fec_encoder->buffers[i],
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

void destroy_fec_encoder(FECEncoder* fec_context_raw) {
    InternalFECEncoder* fec_encoder = (InternalFECEncoder*)fec_context_raw;

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
    fec_decoder->num_accepted_real_buffers = 0;
    fec_decoder->max_packet_size = -1;
    fec_decoder->rs_code = get_rs_code(num_real_buffers, num_real_buffers + num_fec_buffers);
    fec_decoder->recovery_performed = false;
    return fec_decoder;
}

void fec_decoder_register_buffer(FECDecoder* fec_decoder_raw, int index, void* buffer,
                                 int buffer_size) {
    InternalFECDecoder* fec_decoder = (InternalFECDecoder*)fec_decoder_raw;

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

int fec_get_decoded_buffer(FECDecoder* fec_decoder_raw, void* buffer) {
    InternalFECDecoder* fec_decoder = (InternalFECDecoder*)fec_decoder_raw;

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
        int res = fec_decode(fec_decoder->rs_code, (void**)fec_decoder->buffers, index,
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

void destroy_fec_decoder(FECDecoder* fec_context_raw) {
    InternalFECDecoder* fec_decoder = (InternalFECDecoder*)fec_context_raw;

    if (fec_decoder->recovery_performed) {
        for (int i = 0; i < fec_decoder->num_real_buffers; i++) {
            free(fec_decoder->buffers[i]);
        }
    }
    free(fec_decoder->buffers);
    free(fec_decoder->buffer_sizes);
    free(fec_decoder);
}
