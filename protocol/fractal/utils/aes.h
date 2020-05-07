/*
 * AES encryption for streamed packets.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef AES_H
#define AES_H

#include "../core/fractal.h"
#include "../network/network.h"

uint32_t Hash(void* key, size_t len);

int encrypt_packet(FractalPacket* plaintext_packet, int packet_len,
                   FractalPacket* encrypted_packet,
                   unsigned char* private_key);

int decrypt_packet(FractalPacket* encrypted_packet, int packet_len,
                   FractalPacket* plaintext_packet,
                   unsigned char* private_key);

int decrypt_packet_n(FractalPacket* encrypted_packet, int packet_len,
                     FractalPacket* plaintext_packet, int plaintext_len,
                     unsigned char* private_key);

#endif  // AES_H
