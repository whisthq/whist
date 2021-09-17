#ifndef FEC_H
#define FEC_H

typedef void fec_context;

fec_context* create_fec_encoder_context();
fec_context* create_fec_decoder_context(int num_real_buffers, int num_total_buffers);
// Adds a real buffer into the fec context, to be called in order
void encoder_register_buffer(fec_context* fec_encoder, char* buffer, int buffer_size);
// Records a received buffer
void decoder_register_buffer(fec_context* fec_decoder, int index, char* buffer, int buffer_size);
// Gets an array of buffers/buffer_sizes, and sets num_buffers
void get_encoded_buffers(fec_context* fec_encoder, char** buffers, int** buffer_sizes,
                         int* num_buffers);
// Writes into buffer and sets buffer_size
int get_decoded_buffer(fec_context* fec_decoder, char* buffer, int* buffer_size);
void free_fec_context(fec_context* fec_context);

#endif
