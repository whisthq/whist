#ifndef AES_H
#define AES_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>
#include "../network/network.h"

/*
============================
Defines
============================
*/

// Assertions in aes.c verify that these constants are accurate,
// prior to any encrypting or decrypting operations
#define IV_SIZE 16
#define KEY_SIZE 16
#define HMAC_SIZE 16
#define MAX_ENCRYPTION_SIZE_INCREASE 32

/**
 * @brief    The is metadata about the encrypted packet,
 *
 */
typedef struct {
    // Contains a signature of the iv/cipher_len/the encrypted data
    char hmac[HMAC_SIZE];

    // Encrypted packet data
    char iv[IV_SIZE];   // One-time pad for encrypted data
    int encrypted_len;  // The length of the encrypted segment
} AESMetadata;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Hash a buffer to a pseudorandomly unique ID
 *                                 This will not be cryptographically secure against
 *                                 preimage/collision attacks, it is intended to be fast instead
 *
 * @param data                     Pointer to the data to get hashed
 * @param len                      Length of buffer to get hashed, in bytes
 *
 * @returns                        Return the result of the hash
 */
uint32_t hash(const void* data, size_t len);

/**
 * @brief                          Generates a signature of the given buffer
 *                                 This will generate the same signature given
 *                                 the same buf/len, and proves that someone
 *                                 with the private key generated the hash
 *
 * @param hash                     A HMAC_SIZE-byte buffer to write the signature to
 * @param buf                      The a pointer to the buffer that we want to sign
 * @param len                      The length of the bugger that we want to sign
 * @param private_key              The private key to sign with
 *                                   must be KEY_SIZE bytes
 */
void hmac(void* hash, const void* buf, int len, const void* private_key);

/**
 * @brief                          Generates some unique IV,
 *                                 so that a unique IV is given on consective calls
 *
 * @param iv                       A IV_SIZE-byte buffer to write the IV to
 */
void gen_iv(void* iv);

/**
 * @brief                          Encrypts a data packet using the AES private
 *                                 key
 *
 * @param encrypted_data           Pointer to receive the encrypted data
 *                                   NOTE: Buffer must be ENCRYPTION_SIZE_INCREASE bytes larger than
 * packet_len
 * @param aes_metadata             Metadata about the encrypted packet gets into this struct
 * @param plaintext_data           Pointer to the plaintext data to encrypt
 * @param plaintext_len            Length of the packet to encrypt, in bytes
 * @param private_key              AES private key used to encrypt the data
 *                                   must be KEY_SIZE bytes
 *
 * @returns                        Will return the encrypted packet length,
 *                                 (Guaranteed <= plaintext_len + ENCRYPTION_SIZE_INCREASE)
 *                                 or -1 on failure
 */
int encrypt_packet(void* encrypted_data, AESMetadata* aes_metadata, const void* plaintext_data,
                   int plaintext_len, const void* private_key);

/**
 * @brief                          Calls decrypt_packet_n to decrypt an
 *                                 AES-encrypted packet
 *
 * @param plaintext_buffer         Pointer to write the plaintext to
 * @param plaintext_buffer_len     The size that the plaintext_buffer can take in,
 *                                   passed as a parameter to prevent buffer overflows
 * @param aes_metadata             Metadata about the encrypted packet,
 *                                 which is needed for decryption
 * @param encrypted_data           Pointer to the encrypted data
 * @param encrypted_len            The length of the encrypted data buffer
 * @param private_key              AES private key used to encrypt and decrypt
 *                                   must be KEY_SIZE bytes
 *
 * @returns                        Will return the decrypted packet length,
 *                                 or -1 on failure
 *                                   (Either because it was incorrectly signed/encrypted,
 *                                    or if plaintext_buffer_len was too small)
 */
int decrypt_packet(void* plaintext_buffer, int plaintext_buffer_len, AESMetadata aes_metadata,
                   const void* encrypted_data, int encrypted_len, const void* private_key);

#endif  // AES_H
