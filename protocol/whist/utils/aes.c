/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file aes.c
 * @brief This file contains all code that interacts directly with packets
 *        encryption (using AES encryption).
============================
Usage
============================

The function encrypt_packet gets called when a new packet of data needs to be
sent over the network, while decrypt_packet, which calls decrypt_packet_n, gets
called on the receiving end to re-obtain the data and process it.
*/

#if defined(_WIN32)
#pragma warning(disable : 4706)  // assignment within conditional expression warning
#endif

/*
============================
Includes
============================
*/

#include "aes.h"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <time.h>

/*
============================
Defines
============================
*/

// Handles and prints the ssl error,
// Then returns -1
// We LOG_INFO to get the line number
#define HANDLE_SSL_ERROR()                \
    do {                                  \
        LOG_INFO("OpenSSL Error caught"); \
        print_ssl_errors();               \
        return -1;                        \
    } while (0)

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          AES Encrypt plaintext data, given the iv/key
 *                                 This won't sign the packet, however
 *
 * @param ciphertext               Pointer to buffer for receiving ciphertext
 * @param plaintext                Pointer to the plaintext to encrypt
 * @param plaintext_len            Length of the plaintext
 * @param private_key              AES Private Key used to encrypt the plaintext
 *                                   must be KEY_SIZE bytes
 * @param iv                       IV used to seed the AES encryption
 *                                   must be IV_SIZE bytes
 *
 * @returns                        Will return -1 on failure, else will return the
 *                                 length of the encrypted result
 */
static int aes_encrypt(void* ciphertext, const void* plaintext, int plaintext_len,
                       const void* private_key, const void* iv);

/**
 * @brief                          AES Decrypt ciphertext data, given the iv/key
 *
 * @param plaintext_buffer         Pointer to buffer for receiving plaintext
 * @param plaintext_len            The size of the `plaintext` buffer
 * @param ciphertext               Pointer to the ciphertext to encrypt
 * @param ciphertext_len           Length of the ciphertext
 * @param private_key              AES Private Key used to encrypt the plaintext
 *                                   must be KEY_SIZE bytes
 * @param iv                       IV used to seed the AES encryption
 *                                   must be IV_SIZE bytes
 *
 * @returns                        Will return the length of the decrypted result,
 *                                 or -1 on failure
 */
static int aes_decrypt(void* plaintext_buffer, int plaintext_len, const void* ciphertext,
                       int ciphertext_len, const void* private_key, const void* iv);

/**
 * @brief                          Get an HMAC from the AES encrypted information
 *
 * @param hash_buffer              This buffer will have HMAC_SIZE bytes written to it
 * @param aes_metadata             AES Metadata about the encrypted packet,
 *                                   where important information will be signed as well
 * @param ciphertext               Pointer to the ciphertext to sign
 * @param ciphertext_len           Length of the ciphertext
 * @param private_key              The shared private key between client and server
 *                                   must be KEY_SIZE bytes
 */
static void hmac_aes_data(void* hash_buffer, const AESMetadata* aes_metadata,
                          const void* ciphertext, int ciphertext_len, const void* private_key);

/**
 * @brief                          Writes any OpenSSL errors to stderr,
 *                                 and then fatally terminates the protocol
 */
static void print_ssl_errors(void);

/**
 * @brief                          An OpenSSL callback function, see `ERR_print_errors_cb`
 *
 * @param str                     The string for the openssl error
 * @param len                     The length of the string
 * @param opaque                  Ignored
 */
static int openssl_callback(const char* str, size_t len, void* opaque);

/*
============================
Public Function Implementations
============================
*/

uint32_t hash(const void* buf, size_t len) {
    const char* key = buf;

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

void hmac(void* hash, const void* buf, int len, const void* private_key) {
    /*
        https://www.openssl.org/docs/man1.1.1/man3/HMAC.html
        It places the result in md (which must have space for the output of the hash function, which
       is no more than EVP_MAX_MD_SIZE bytes).
    */

    // Temporary buffer that's guaranteed to not overflow
    char hash_buffer[EVP_MAX_MD_SIZE];

    // Hash, and get the hash_len
    int hash_len;
    HMAC(EVP_sha256(), private_key, KEY_SIZE, (const unsigned char*)buf, len,
         (unsigned char*)hash_buffer, (unsigned int*)&hash_len);

    // Verify that the hash_len gave us enough bytes,
    // to confirm the security-level that we wanted
    FATAL_ASSERT(hash_len >= HMAC_SIZE);

    // But we only want to write HMAC_SIZE bytes to the output buffer
    memcpy(hash, hash_buffer, HMAC_SIZE);
}

void gen_iv(void* iv) {
    // Generally vaguely random things,
    // this doesn't need to be secure or anything,
    // the IV will be publically visible anyway.
    // It only needs to be unique on consecutive calls
    srand((unsigned int)time(NULL) * rand() + rand());
    (void)rand();
    srand((unsigned int)time(NULL) * rand() + rand());
    (void)rand();
    (void)rand();

    for (int i = 0; i < IV_SIZE; i++) {
        ((unsigned char*)iv)[i] = (unsigned char)rand();
    }
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int encrypt_packet(void* encrypted_data, AESMetadata* aes_metadata, const void* plaintext_data,
                   int plaintext_len, const void* private_key) {
    // A unique random number so that all packets are encrypted uniquely
    // (So that e.g. same plaintext twice gives unique encrypted packets)
    gen_iv(aes_metadata->iv);

    // Encrypt the data, and store the length in the metadata
    int encrypted_len =
        aes_encrypt(encrypted_data, plaintext_data, plaintext_len, private_key, aes_metadata->iv);
    aes_metadata->encrypted_len = encrypted_len;

    // Hash into the aes metadata's HMAC field
    hmac_aes_data(aes_metadata->hmac, aes_metadata, encrypted_data, encrypted_len, private_key);

    // Return the length of the encrypted buffer
    return encrypted_len;
}

int decrypt_packet(void* plaintext_buffer, int plaintext_buffer_len, AESMetadata aes_metadata,
                   const void* encrypted_data, int encrypted_len, const void* private_key) {
    if (aes_metadata.encrypted_len != encrypted_len) {
        LOG_WARNING("The received AESMetadata doesn't match the size of the encrypted data");
        return -1;
    }

    // Get the expected HMAC
    char expected_hmac[HMAC_SIZE];
    hmac_aes_data(expected_hmac, &aes_metadata, encrypted_data, encrypted_len, private_key);

    // Verify that HMACs match
    if (memcmp(aes_metadata.hmac, expected_hmac, sizeof(aes_metadata.hmac)) != 0) {
        LOG_WARNING("Incorrect hmac!");
        return -1;
    }

    // Decrypt the packet using the private key, and the metadata's IV
    int decrypt_len = aes_decrypt(plaintext_buffer, plaintext_buffer_len, encrypted_data,
                                  aes_metadata.encrypted_len, private_key, aes_metadata.iv);

    // And then returnt he decrypted packet, passing on -1 as well
    return decrypt_len;
}

/*
============================
Private Function Implementations
============================
*/

int aes_encrypt(void* ciphertext, const void* plaintext, int plaintext_len, const void* key,
                const void* iv) {
    EVP_CIPHER_CTX* ctx;
    const EVP_CIPHER* cipher = EVP_aes_128_cbc();

    int len;

    int ciphertext_buffer_size = plaintext_len + MAX_ENCRYPTION_SIZE_INCREASE;

    int ciphertext_bytes_written = 0;

    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new())) HANDLE_SSL_ERROR();

    // Verify the constants of aes.h before usage
    FATAL_ASSERT(IV_SIZE == EVP_CIPHER_iv_length(cipher));
    FATAL_ASSERT(KEY_SIZE == EVP_CIPHER_key_length(cipher));
    int block_size = EVP_CIPHER_block_size(cipher);
    FATAL_ASSERT(MAX_ENCRYPTION_SIZE_INCREASE >= block_size);

    // Initialise the encryption operation.
    if (1 !=
        EVP_EncryptInit_ex(ctx, cipher, NULL, (const unsigned char*)key, (const unsigned char*)iv))
        HANDLE_SSL_ERROR();

    /*
        https://www.openssl.org/docs/man1.1.1/man3/EVP_EncryptUpdate.html
        For wrap cipher modes, the amount of data written can be anything from zero bytes to (inl +
       cipher_block_size) bytes. For stream ciphers, the amount of data written can be anything from
       zero bytes to inl bytes. Thus, out should contain sufficient room for the operation being
       performed.

        Verification that no buffer overflow can happen occurs in the following FATAL_ASSERT
    */

    // Size of the remaining ciphertext buffer, must be >= inl + block_size
    FATAL_ASSERT(ciphertext_buffer_size - ciphertext_bytes_written >= plaintext_len + block_size);

    // Encrypt
    if (1 != EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext + ciphertext_bytes_written, &len,
                               (const unsigned char*)plaintext, plaintext_len))
        HANDLE_SSL_ERROR();
    ciphertext_bytes_written += len;

    /*
        https://www.openssl.org/docs/man1.1.1/man3/EVP_EncryptUpdate.html
        The encrypted final data is written to out which should have sufficient space for one cipher
       block. The number of bytes written is placed in outl.

        Verification that no buffer overflow can happen occurs in the following FATAL_ASSERT
    */

    // Size of the remaining ciphertext buffer, must be >= block_size
    FATAL_ASSERT(ciphertext_buffer_size - ciphertext_bytes_written >= block_size);

    // Finish encryption
    if (1 != EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext + ciphertext_bytes_written, &len))
        HANDLE_SSL_ERROR();
    ciphertext_bytes_written += len;

    // Free the context
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_bytes_written;
}

int aes_decrypt(void* plaintext_buffer, int plaintext_len, const void* ciphertext,
                int ciphertext_len, const void* key, const void* iv) {
    EVP_CIPHER_CTX* ctx;
    const EVP_CIPHER* cipher = EVP_aes_128_cbc();

    int len;

    int plaintext_bytes_written = 0;
    int ciphertext_bytes_read = 0;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) HANDLE_SSL_ERROR();

    // Verify the constants of aes.h before usage
    FATAL_ASSERT(IV_SIZE == EVP_CIPHER_iv_length(cipher));
    FATAL_ASSERT(KEY_SIZE == EVP_CIPHER_key_length(cipher));
    int block_size = EVP_CIPHER_block_size(cipher);

    // Initialize decryption
    if (1 !=
        EVP_DecryptInit_ex(ctx, cipher, NULL, (const unsigned char*)key, (const unsigned char*)iv))
        HANDLE_SSL_ERROR();

    /*
        https://www.openssl.org/docs/man1.1.1/man3/EVP_EncryptUpdate.html
        The parameters and restrictions are identical to the encryption operations except that if
       padding is enabled the decrypted data buffer out passed to EVP_DecryptUpdate() should have
       sufficient room for (inl + cipher_block_size) bytes unless the cipher block size is 1 in
       which case inl bytes is sufficient.

        Verification that no buffer overflow can happen occurs here
    */

    // The below code will EVP_DecryptUpdate most of the bytes,
    // writing it directly into plaintext_buffer rather than an intermediate buffer

    // Can't set inl to be within block_size of plaintext_len,
    // And also can't set inl to be more than ciphertext_len
    int safe_plaintext_len = min(max(plaintext_len - block_size, 0), ciphertext_len);
    if (safe_plaintext_len > 0) {
        if (1 != EVP_DecryptUpdate(ctx, (unsigned char*)plaintext_buffer, &len,
                                   (const unsigned char*)ciphertext, safe_plaintext_len))
            HANDLE_SSL_ERROR();

        ciphertext_bytes_read += safe_plaintext_len;
        plaintext_bytes_written += len;
    }

    // Temp buf to store partially decrypted information,
    // so that we decrypt 64bytes at a time
    unsigned char temporary_buf[64 + EVP_MAX_BLOCK_LENGTH];

    // Decrypt any remaining data, in small blocks at a time
    // While this code can do all of the work if we wanted,
    // We use EVP_DecryptUpdate above to save a memcpy of the primary bulk of the data,
    // because the code above doesn't use a `temporary_buf` buffer.
    while (ciphertext_bytes_read < ciphertext_len) {
        // Decrypt into temporary_buf, but not within block_size of the size of temporary_buf,
        // And not more than the # of bytes left in the ciphertext
        int bytes_to_feed =
            min(ciphertext_len - ciphertext_bytes_read, (int)sizeof(temporary_buf) - block_size);
        if (1 != EVP_DecryptUpdate(ctx, temporary_buf, &len,
                                   (const unsigned char*)ciphertext + ciphertext_bytes_read,
                                   bytes_to_feed))
            HANDLE_SSL_ERROR();
        ciphertext_bytes_read += bytes_to_feed;

        // Move from temporary_buf to plaintext_buffer, if we have the room available
        if (plaintext_bytes_written + len > plaintext_len) {
            LOG_INFO("We want to write %d bytes, but we only have %d bytes",
                     plaintext_bytes_written + len, plaintext_len);
            return -1;
        }
        memcpy((char*)plaintext_buffer + plaintext_bytes_written, temporary_buf, len);
        plaintext_bytes_written += len;
    }

    // Finish decryption
    if (1 != EVP_DecryptFinal_ex(ctx, temporary_buf, &len)) HANDLE_SSL_ERROR();

    // Move from temporary_buf to plaintext_buffer, if we have the room available
    if (plaintext_bytes_written + len > plaintext_len) {
        LOG_INFO("We want to write %d bytes, but we only have %d bytes",
                 plaintext_bytes_written + len, plaintext_len);
        return -1;
    }
    memcpy((char*)plaintext_buffer + plaintext_bytes_written, temporary_buf, len);
    plaintext_bytes_written += len;

    // Free context
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_bytes_written;
}

void hmac_aes_data(void* hash_buffer, const AESMetadata* aes_metadata, const void* ciphertext,
                   int ciphertext_len, const void* private_key) {
    // Use a different private key for HMAC
    unsigned char hmac_private_key[KEY_SIZE];
    FATAL_ASSERT(KEY_SIZE == HMAC_SIZE);
    // This writes HMAC_SIZE bytes, which ends up being KEY_SIZE bytes anyway
    hmac(hmac_private_key, private_key, KEY_SIZE, DEFAULT_BINARY_PRIVATE_KEY);

    // Contains the concatenated signatures
    char concatenated_hash[HMAC_SIZE + HMAC_SIZE];

    // Sign aes_metadata into the first HMAC_SIZE-bytes
    hmac(&concatenated_hash[0], (char*)aes_metadata + sizeof(aes_metadata->hmac),
         sizeof(*aes_metadata) - sizeof(aes_metadata->hmac), hmac_private_key);

    // Sign the encrypted data into the next HMAC_SIZE-bytes
    hmac(&concatenated_hash[HMAC_SIZE], ciphertext, ciphertext_len, hmac_private_key);

    // Combine the two into a single hmac
    hmac(hash_buffer, concatenated_hash, sizeof(concatenated_hash), hmac_private_key);
}

static void print_ssl_errors(void) { ERR_print_errors_cb(openssl_callback, NULL); }

static int openssl_callback(const char* str, size_t len, void* opaque) {
    UNUSED(opaque);
    LOG_ERROR("OpenSSL Error: %.*s\n", (int)len, str);

    return 0;
}

#if defined(_WIN32)
#pragma warning(default : 4706)
#endif
