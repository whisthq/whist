#ifndef AES_H
#define AES_H
/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <fractal/core/fractal.h>
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
uint32_t hash(void* key, size_t len);

/**
 * @brief                          Generate a random IV
 *
 * @param iv                       Buffer to save the IV to
 */
void gen_iv(void* iv);

/**
 * @brief                          Generate an hmac signature
 *
 * @param signature                Buffer to save the signature to
 * @param buf                      Buffer to sign
 * @param len                      Length of buffer to sign
 * @param key                      Private key to sign with
 */
int hmac(void* signature, void* buf, int len, void* key);

/**
 * @brief                          Verify an hmac signature
 *
 * @param signature                Signature of the buffer
 * @param buf                      Buffer that is signed
 * @param len                      Length of signed buffer
 * @param key                      Private key that was signed with
 */
bool verify_hmac(void* signature, void* buf, int len, void* key);

/**
 * @brief                          Encrypts a data packet using the AES private
 *                                 key
 *
 * @param plaintext_packet         Pointer to the plaintext packet to be
 *                                 encrypted
 * @param packet_len               Length of the packet to encrypt, in bytes
 * @param encrypted_packet         Pointer to receive the encrypted packet
 * @param private_key              AES private key used to encrypt and decrypt
 *                                 the packets
 *
 * @returns                        Will return the encrypted packet length
 */
int encrypt_packet(FractalPacket* plaintext_packet, int packet_len, FractalPacket* encrypted_packet,
                   unsigned char* private_key);

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
 * @returns                        Will return -1 on failure, else will return the
 *                                 length of the decrypted packets
 */
int decrypt_packet(FractalPacket* encrypted_packet, int packet_len, FractalPacket* plaintext_packet,
                   unsigned char* private_key);

/**
 * @brief                          Decrypts an AES-encrypted packet
 *
 * @param encrypted_packet         Pointer to the encrypted packet to be
 *                                 decrypted
 * @param packet_len               Length of the packet to decrypt, in bytes
 * @param plaintext_packet         Pointer to receive the decrypted packet
 * @param plaintext_len     Length of the decrypted packet, in bytes
 * @param private_key              AES private key used to encrypt and decrypt
 *                                 the packets
 *
 * @returns                        Will return -1 on failure, else will return the
 *                                 length of the decrypted packets
 */
int decrypt_packet_n(FractalPacket* encrypted_packet, int packet_len, FractalPacket* plaintext_packet,
                     int plaintext_len, unsigned char* private_key);

/**
 * @brief                          Encrypt plaintext data
 *
 * @param plaintext                Pointer to the plaintext to encrypt
 * @param plaintext_len            Length of the plaintext
 * @param key                      AES Private Key used to encrypt the plaintext
 * @param iv                       IV used to seed the AES encryption
 * @param ciphertext               Pointer to buffer for receiving ciphertext
 *
 * @returns                        Will return -1 on failure, else will return the
 *                                 length of the encrypted result
 */
int aes_encrypt(void* plaintext, int plaintext_len, void* key, void* iv, void* ciphertext);

/**
 * @brief                          Decrypt ciphertext data
 *
 * @param ciphertext               Pointer to the ciphertext to encrypt
 * @param ciphertext_len           Length of the ciphertext
 * @param key                      AES Private Key used to encrypt the plaintext
 * @param iv                       IV used to seed the AES encryption
 * @param plaintext                Pointer to buffer for receiving plaintext
 *
 * @returns                        Will return -1 on failure, else will return the
 *                                 length of the decrypted result
 */
int aes_decrypt(void* ciphertext, int ciphertext_len, void* key, void* iv, void* plaintext);

#endif  // AES_H
