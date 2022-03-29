#ifndef FEC_H
#define FEC_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file fec.h
 * @brief This file contains the FEC API Interface
 * @note See protocol_test.cpp for usage
 */

/*
============================
Defines
============================
*/

// This is unused at the moment
#define FEC_HEADER_SIZE 2

typedef struct FECEncoder FECEncoder;
typedef struct FECDecoder FECDecoder;

// temp workaround of for cm256 lib's CPU dispatching
// TODO: fix cm256
#define WHIST_ERROR_AVX2_NOT_SUPPORTED -100
/*
============================
Public Functions
============================
*/

/**
 * @brief                          A global initialization function that should
 *                                 be called before calling anything else in fec.h
 * @returns                        zero on success
 */
int init_fec(void);

/**
 * @brief                          Converts an fec packet ratio,
 *                                 into the number of fec packets.
 *
 * @param num_real_packets         The numbers of packets used in the original frame
 *
 * @param fec_packet_ratio         The ratio of packets that should be FEC
 *
 * @returns                        The number of fec packets
 */
int get_num_fec_packets(int num_real_packets, double fec_packet_ratio);

/**
 * @brief                          Creates an FEC Encoder
 *
 * @param num_real_buffers         The numbers of buffers used in the original frame
 *
 * @param num_fec_buffers          The number of additional buffers we want to have
 *                                 after encoding with FEC
 *
 * @param max_buffer_size          The largest valid size of a buffer
 *
 * @returns                        The initialized FEC Encoder
 */
FECEncoder* create_fec_encoder(int num_real_buffers, int num_fec_buffers, int max_buffer_size);

/**
 * @brief                          Calculate the num of real buffers needed, to hold the
 *                                 (input) buffer
 *
 * @param buffer_size              The buffer size that is going to feed into
 *                                 fec_encoder_register_buffer()
 *
 * @param real_buffer_size         The real_buffer_size allowed
 *
 * @note                           Think this as spliting a large buffer into smaller ones. Since
 *                                 the exist of FEC padding, the calculate is non-trival.
 *
 * @returns                        The num of real buffers needed
 */
int fec_encoder_get_num_real_buffers(int buffer_size, int real_buffer_size);

/**
 * @brief                          Registers a buffer into the encoder.
 *
 * @param fec_encoder              The FEC encoder to use
 *
 * @param buffer                   The buffer to register
 *
 * @param buffer_size              The size of the buffer that's being registered,
 *                                 must be <= `max_buffer_size`*`num_real_buffers`
 *
 * @note                           The data pointed to by the buffer being passed in, which
 *                                 must be held alive for as long as the fec_encoder is alive.
 */
void fec_encoder_register_buffer(FECEncoder* fec_encoder, void* buffer, int buffer_size);

/**
 * @brief                          Gets the encoded buffers
 *
 * @param fec_encoder              The FEC encoder to use
 *
 * @param buffers                  The array of `void*` buffers for the FEC encoded packets
 *
 * @param buffer_sizes             The array of `int` buffer sizes of the FEC encoded packets
 *
 * @note                           The two arrays given should be allocated to be of size
 *                                 `num_real_buffers` + `num_fec_buffers`.
 *                                 Additionally, the data pointed to by the `void*`'s will be
 *                                 alive for as long as the FEC encoder is alive.
 */
void fec_get_encoded_buffers(FECEncoder* fec_encoder, void** buffers, int* buffer_sizes);

/**
 * @brief                          Destroys an FEC Encoder
 *
 * @param fec_encoder              The FEC encoder to destroy
 *
 * @note                           Should be called for every `create_fec_encoder`
 */
void destroy_fec_encoder(FECEncoder* fec_encoder);

/**
 * @brief                          Creates an FEC Decoder
 *
 * @param num_real_buffers         The numbers of buffers used in the original frame
 *
 * @param num_fec_buffers          The number of additional fec buffers that were used
 *
 * @param max_buffer_size          The largest valid size of a buffer
 *
 * @returns                        The initialized FEC Decoder
 *
 * @note                           When decoding, these these three parameters MUST match
 *                                 the three parameters passed into `create_fec_encoder`
 */
FECDecoder* create_fec_decoder(int num_real_buffers, int num_fec_buffers, int max_buffer_size);

/**
 * @brief                          Registers a buffer into the decoder.
 *
 * @param fec_decoder              The FEC decoder to use
 *
 * @param index                    The index of the buffer that's being registered.
 *                                 Must be between 0 and `num_real_buffers + num_fec_buffers` - 1
 *
 * @param buffer                   The buffer to register
 *
 * @param buffer_size              The size of the buffer that's being registered
 *
 * @note                           All buffers passed into this function should be some subset
 *                                 of the buffers returned by `fec_get_encoded_buffers`, with
 *                                 the index indexing into the arrays returned by that function.
 *
 *                                 The data pointed to by the buffer being passed in, must be
 *                                 held alive for as long as the fec_decoder is alive.
 */
void fec_decoder_register_buffer(FECDecoder* fec_decoder, int index, void* buffer, int buffer_size);

/**
 * @brief                          Registers a buffer into the decoder.
 *
 * @param fec_decoder              The FEC decoder to use
 *
 * @param buffer                   The buffer to write the decoded frame into
 *                                 The buffer must be set to a buffer at least the size of
 *                                 `num_real_buffers * max_buffer_size`,
 *                                 or `NULL` if you only want the return value of this function.
 *
 * @returns                        The size of the decoded buffer,
 *                                 or -1 if it was not possible to decode it
 *
 * @note                           This refers to some buffer returned by `fec_get_encoded_buffers`,
 *                                 with `index` indexing into the arrays returned by that function.
 */
int fec_get_decoded_buffer(FECDecoder* fec_decoder, void* buffer);

/**
 * @brief                          Destroy an FEC Decoder
 *
 * @param fec_decoder              The FEC decoder to destroy
 *
 * @note                           Should be called for every `create_fec_decoder`
 */
void destroy_fec_decoder(FECDecoder* fec_decoder);

#endif
