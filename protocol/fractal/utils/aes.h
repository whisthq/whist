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

int encrypt_packet(struct RTPPacket* plaintext_packet, int packet_len,
                   struct RTPPacket* encrypted_packet,
                   unsigned char* private_key);

int decrypt_packet(struct RTPPacket* encrypted_packet, int packet_len,
                   struct RTPPacket* plaintext_packet,
                   unsigned char* private_key);

int decrypt_packet_n(struct RTPPacket* encrypted_packet, int packet_len,
                     struct RTPPacket* plaintext_packet, int plaintext_len,
                     unsigned char* private_key);

#endif  // AES_H
