/*
 * AES encryption for streamed packets.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#pragma warning( \
    disable : 4706)  // assignment within conditional expression warning
#endif

#include "aes.h"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

uint32_t Hash(void* buf, size_t len) {
    char* key = buf;

    uint64_t pre_hash = 123456789;
    while (len > 8) {
        pre_hash += *(uint64_t*)(key);
        pre_hash += (pre_hash << 32);
        pre_hash += (pre_hash >> 32);
        pre_hash += (pre_hash << 10);
        pre_hash ^= (pre_hash >> 6);
        pre_hash ^= (pre_hash << 48);
        pre_hash ^= 123456789;
        len -= 8;
        key += 8;
    }

    uint32_t hash = (uint32_t)((pre_hash << 32) ^ pre_hash);
    for (size_t i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

int aes_encrypt(unsigned char* plaintext, int plaintext_len, unsigned char* key,
                unsigned char* iv, unsigned char* ciphertext);
int aes_decrypt(unsigned char* ciphertext, int ciphertext_len,
                unsigned char* key, unsigned char* iv,
                unsigned char* plaintext);

void handleErrors(void) {
    ERR_print_errors_fp(stderr);
    abort();
}

void gen_iv(unsigned char* iv) {
    srand((unsigned int)time(NULL) * rand() + rand());
    (void)rand();
    (void)rand();
    (void)rand();

    for (int i = 0; i < 16; i++) {
        iv[i] = (unsigned char)rand();
    }
}

int hmac(char* hash, char* buf, int len, char* key) {
    int hash_len;
    HMAC(EVP_sha256(), key, 16, (const unsigned char*)buf, len,
         (unsigned char*)hash, (unsigned int*)&hash_len);
    if (hash_len != 32) {
        LOG_WARNING("Incorrect hash length!");
        return -1;
    }
    return hash_len;
}

#define CRYPTO_HEADER_LEN                                                    \
    (sizeof(plaintext_packet->hash) + sizeof(plaintext_packet->cipher_len) + \
     sizeof(plaintext_packet->iv))

int encrypt_packet(FractalPacket* plaintext_packet, int packet_len,
                   FractalPacket* encrypted_packet,
                   unsigned char* private_key) {
    char* plaintext_buf = (char*)plaintext_packet + CRYPTO_HEADER_LEN;
    int plaintext_buf_len = packet_len - CRYPTO_HEADER_LEN;
    // A unique random number so that all packets are encrypted uniquely
    // (Same plaintext twice gives unique encrypted packets)
    gen_iv((unsigned char*)encrypted_packet->iv);

    char* cipher_buf = (char*)encrypted_packet + CRYPTO_HEADER_LEN;
    int cipher_len = aes_encrypt(
        (unsigned char*)plaintext_buf, plaintext_buf_len, private_key,
        (unsigned char*)encrypted_packet->iv, (unsigned char*)cipher_buf);
    encrypted_packet->cipher_len = cipher_len;

    int cipher_packet_len = cipher_len + CRYPTO_HEADER_LEN;

    // mprintf( "HMAC: %d\n", Hash( encrypted_packet->hash, 16 ) );
    char hash[32];
    // Sign the packet with 32 bytes
    hmac(hash, (char*)encrypted_packet + sizeof(encrypted_packet->hash),
         cipher_packet_len - sizeof(encrypted_packet->hash),
         (char*)private_key);
    // Only use 16 bytes bc we don't need that long of a signature
    memcpy(encrypted_packet->hash, hash, 16);
    // mprintf( "HMAC: %d\n", Hash( encrypted_packet->hash, 16 ) );
    // encrypted_packet->hash = Hash( (char*)encrypted_packet + sizeof(
    // encrypted_packet->hash ), cipher_packet_len - sizeof(
    // encrypted_packet->hash ) );

    return cipher_packet_len;
}

int decrypt_packet(FractalPacket* encrypted_packet, int packet_len,
                   FractalPacket* plaintext_packet,
                   unsigned char* private_key) {
    if ((unsigned long)packet_len > MAX_PACKET_SIZE) {
        LOG_WARNING("Encrypted version of Packet is too large!");
        return -1;
    }
    int decrypt_len =
        decrypt_packet_n(encrypted_packet, packet_len, plaintext_packet,
                         PACKET_HEADER_SIZE + MAX_PAYLOAD_SIZE, private_key);
    return decrypt_len;
}

int decrypt_packet_n(FractalPacket* encrypted_packet, int packet_len,
                     FractalPacket* plaintext_packet, int plaintext_len,
                     unsigned char* private_key) {
    if ((unsigned long)packet_len < PACKET_HEADER_SIZE) {
        LOG_WARNING("Packet is too small for metadata!");
        return -1;
    }

    char hash[32];
    hmac(hash, (char*)encrypted_packet + sizeof(encrypted_packet->hash),
         packet_len - sizeof(encrypted_packet->hash), (char*)private_key);
    for (int i = 0; i < 16; i++) {
        if (hash[i] != encrypted_packet->hash[i]) {
            LOG_WARNING("HMAC failed!");
            return -1;
        }
    }

    char* cipher_buf = (char*)encrypted_packet + CRYPTO_HEADER_LEN;
    char* plaintext_buf = (char*)plaintext_packet + CRYPTO_HEADER_LEN;

    int decrypt_len = aes_decrypt(
        (unsigned char*)cipher_buf, encrypted_packet->cipher_len, private_key,
        (unsigned char*)encrypted_packet->iv, (unsigned char*)plaintext_buf);
    decrypt_len += CRYPTO_HEADER_LEN;

    int expected_len = PACKET_HEADER_SIZE + plaintext_packet->payload_size;
    if (expected_len != decrypt_len) {
        mprintf(
            "Packet length is incorrect! Expected %d with payload %d, but got "
            "%d\n",
            expected_len, plaintext_packet->payload_size, decrypt_len);
        return -1;
    }

    if (decrypt_len > plaintext_len) {
        LOG_WARNING("Decrypted version of Packet is too large!");
        return -1;
    }

    return decrypt_len;
}

int aes_encrypt(unsigned char* plaintext, int plaintext_len, unsigned char* key,
                unsigned char* iv, unsigned char* ciphertext) {
    EVP_CIPHER_CTX* ctx;

    int len;

    int ciphertext_len;

    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    // Initialise the encryption operation.
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    // Encrypt
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    // Finish encryption (Might add a few bytes)
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &len))
        handleErrors();
    ciphertext_len += len;

    // Free the context
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int aes_decrypt(unsigned char* ciphertext, int ciphertext_len,
                unsigned char* key, unsigned char* iv,
                unsigned char* plaintext) {
    EVP_CIPHER_CTX* ctx;

    int len;

    int plaintext_len;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    // Initialize decryption
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    // Decrypt
    if (1 !=
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    // Finish decryption
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
    plaintext_len += len;

    // Free context
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

#if defined(_WIN32)
#pragma warning(default : 4706)
#endif
