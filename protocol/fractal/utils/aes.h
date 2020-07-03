#ifndef AES_H
#define AES_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file aes.h
 * @brief This file contains all code that interacts directly with packets
 *        encryption (using AES encryption).
============================
Usage
============================

The function encrypt_packet gets called when a new packet of data needs to be
sent over the network, while decrypt_packet, which calls decrypt_packet_n, gets
called on the receiving end to re-obtain the data and process it.
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"
#include "../network/network.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Hash a buffer to a pseudorandomly unique ID
 *
 * @param key                      Buffer to get hashed
 * @param len                      Length of buffer to get hashed, in bytes
 *
 * @returns                        Return the result of the hash
 */
uint32_t Hash(void* key, size_t len);

/**
 * @brief                          Encrypts a data packet using the AES private
 *                                 key
 *
 * @param plaintext_packet         Pointer to the plaintext packet to be
 *                                 encrypted
 * @param packet_len               Length of the packet to encrypt, in bytes
 * @param plaintext_packet         Pointer to receive the encrypted packet
 * @param private_key              AES private key used to encrypt and decrypt
 *                                 the packets
 *
 * @returns                        Will return the encrypted packet length
 */
int encrypt_packet(FractalPacket* plaintext_packet, int packet_len,
                   FractalPacket* encrypted_packet, unsigned char* private_key);

/**
 * @brief                          Calls decrypt_packet_n to decrypt an
 *                                 AES-encrypted packet
 *
 * @param encrypted_packet         Pointer to the encrypted packet to be
 *                                 decrypted
 * @param packet_len               Length of the packet to decrypt, in bytes
 * @param plaintext_packet         Pointer to receive the decrypted packet
 * @param private_key              AES private key used to encrypt and decrypt
 *                                 the packets
 *
 * @returns                        Will return -1 on failure, will return the
 *                                 length of the decrypted packets
 */
int decrypt_packet(FractalPacket* encrypted_packet, int packet_len,
                   FractalPacket* plaintext_packet, unsigned char* private_key);

/**
 * @brief                          Decrypts an AES-encrypted packet
 *
 * @param encrypted_packet         Pointer to the encrypted packet to be
 *                                 decrypted
 * @param packet_len               Length of the packet to decrypt, in bytes
 * @param plaintext_packet         Pointer to receive the decrypted packet
 * @param plaintext_packet_len     Length of the decrypted packet, in bytes
 * @param private_key              AES private key used to encrypt and decrypt
 *                                 the packets
 *
 * @returns                        Will return -1 on failure, will return the
 *                                 length of the decrypted packets
 */
int decrypt_packet_n(FractalPacket* encrypted_packet, int packet_len,
                     FractalPacket* plaintext_packet, int plaintext_len,
                     unsigned char* private_key);

#endif  // AES_H
