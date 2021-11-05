#ifndef FEC_H
#define FEC_H

#include <stdbool.h>

typedef void FECEncoder;
typedef void FECDecoder;

// Get the number of FEC packets from a given number of real packets and FEC Packet ratios
int get_num_fec_packets(int num_real_packets, double fec_packet_ratio);

// Create FEC Encoder
FECEncoder* create_fec_encoder(int num_real_buffers, int num_fec_buffers, int packet_size);
// Adds a real buffer into the fec context, to be called in order
void encoder_register_buffer(FECEncoder* fec_encoder, char* buffer, int buffer_size);
// Gets an array of buffers/buffer_sizes, and sets num_buffers
void get_encoded_buffers(FECEncoder* fec_encoder, char** buffers, int** buffer_sizes);
// Free FEC Encoder
void free_fec_encoder(FECEncoder* fec_encoder);

// Create FEC Decoder
FECDecoder* create_fec_decoder(int num_real_buffers, int num_total_buffers, int packet_size);
// Records a received buffer
void decoder_register_buffer(FECDecoder* fec_decoder, int index, char* buffer, int buffer_size);
// Writes the reconstructed buffer into the given buffer, and sets buffer_size
bool get_decoded_buffer(FECDecoder* fec_decoder, char* buffer, int* buffer_size);
// Free FEC Decoder
void free_fec_decoder(FECEncoder* fec_decoder);

#endif
