#include "aes.h"

#include "openssl/conf.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "openssl/hmac.h"

aes_encrypt( unsigned char* plaintext, int plaintext_len, unsigned char* key,
         unsigned char* iv, unsigned char* ciphertext );
aes_decrypt( unsigned char* ciphertext, int ciphertext_len, unsigned char* key,
         unsigned char* iv, unsigned char* plaintext );

void handleErrors( void )
{
    ERR_print_errors_fp( stderr );
    abort();
}

void gen_iv( unsigned char* iv )
{
    int rc = RAND_bytes( iv, 16 );
}

int hmac( char* hash, char* buf, int len, char* key )
{
    int hash_len;
    HMAC( EVP_sha256(), key, 16, buf, len, hash, &hash_len );
    if( hash_len != 32 )
    {
        mprintf( "Incorrect hash length!\n" );
        return -1;
    }
    return hash_len;
}

#define CRYPTO_HEADER_LEN (sizeof( plaintext_packet->hash ) + sizeof(plaintext_packet->cipher_len ) + sizeof( plaintext_packet->iv ))

int encrypt_packet( struct RTPPacket* plaintext_packet, int packet_len, struct RTPPacket* encrypted_packet, unsigned char* private_key)
{
    char* plaintext_buf = (char*)plaintext_packet + CRYPTO_HEADER_LEN;
    int plaintext_buf_len = packet_len - CRYPTO_HEADER_LEN;
    gen_iv( encrypted_packet->iv );

    char* cipher_buf = (char*)encrypted_packet + CRYPTO_HEADER_LEN;
    int cipher_len = aes_encrypt( plaintext_buf, plaintext_buf_len, private_key, encrypted_packet->iv, cipher_buf );
    encrypted_packet->cipher_len = cipher_len;

    int cipher_packet_len = cipher_len + CRYPTO_HEADER_LEN;

    //mprintf( "HMAC: %d\n", Hash( encrypted_packet->hash, 16 ) );
    char hash[32];
    hmac( hash, (char*)encrypted_packet + sizeof( encrypted_packet->hash ), cipher_packet_len - sizeof( encrypted_packet->hash ), PRIVATE_KEY );
    memcpy( encrypted_packet->hash, hash, 16 );
    //mprintf( "HMAC: %d\n", Hash( encrypted_packet->hash, 16 ) );
    //encrypted_packet->hash = Hash( (char*)encrypted_packet + sizeof( encrypted_packet->hash ), cipher_packet_len - sizeof( encrypted_packet->hash ) );

    return cipher_packet_len;
}

int decrypt_packet( struct RTPPacket* encrypted_packet, int packet_len, struct RTPPacket* plaintext_packet, unsigned char* private_key )
{
    if( packet_len > MAX_PACKET_SIZE )
    {
        mprintf( "Encrypted version of Packet is too large!\n" );
        return -1;
    }
    int decrypt_len = decrypt_packet_n( encrypted_packet, packet_len, plaintext_packet, PACKET_HEADER_SIZE + MAX_PAYLOAD_SIZE, private_key );
    return decrypt_len;
}

int decrypt_packet_n( struct RTPPacket* encrypted_packet, int packet_len, struct RTPPacket* plaintext_packet, int plaintext_len, unsigned char* private_key )
{
    if( packet_len < PACKET_HEADER_SIZE )
    {
        mprintf( "Packet is too small for metadata!\n" );
        return -1;
    }

    char hash[32];
    hmac( hash, (char*)encrypted_packet + sizeof( encrypted_packet->hash ), packet_len - sizeof( encrypted_packet->hash ), PRIVATE_KEY );
    for( int i = 0; i < 16; i++ )
    {
        if( hash[i] != encrypted_packet->hash[i] )
        {
            mprintf( "HMAC failed!\n" );
            return -1;
        }
    }

    char* cipher_buf = (char*)encrypted_packet + CRYPTO_HEADER_LEN;
    char* plaintext_buf = (char*)plaintext_packet + CRYPTO_HEADER_LEN;

    int decrypt_len = aes_decrypt( cipher_buf, encrypted_packet->cipher_len, private_key,
                               encrypted_packet->iv, plaintext_buf );
    decrypt_len += CRYPTO_HEADER_LEN;

    int expected_len = PACKET_HEADER_SIZE + plaintext_packet->payload_size;
    if( expected_len != decrypt_len )
    {
        mprintf( "Packet length is incorrect! Expected %d with payload %d, but got %d\n", expected_len, plaintext_packet->payload_size, decrypt_len );
        return -1;
    }

    if( decrypt_len > plaintext_len )
    {
        mprintf( "Decrypted version of Packet is too large!\n" );
        return -1;
    }

    return decrypt_len;
}

int aes_encrypt( unsigned char* plaintext, int plaintext_len, unsigned char* key,
             unsigned char* iv, unsigned char* ciphertext )
{
    EVP_CIPHER_CTX* ctx;

    int len;

    int ciphertext_len;

    // Create and initialise the context
    if( !(ctx = EVP_CIPHER_CTX_new()) )
        handleErrors();

    // Initialise the encryption operation.
    if( 1 != EVP_EncryptInit_ex( ctx, EVP_aes_128_cbc(), NULL, key, iv ) )
        handleErrors();

    // Encrypt
    if( 1 != EVP_EncryptUpdate( ctx, ciphertext, &len, plaintext, plaintext_len ) )
        handleErrors();
    ciphertext_len = len;

    // Finish encryption (Might add a few bytes)
    if( 1 != EVP_EncryptFinal_ex( ctx, ciphertext + ciphertext_len, &len ) )
        handleErrors();
    ciphertext_len += len;

    // Free the context
    EVP_CIPHER_CTX_free( ctx );

    return ciphertext_len;
}


int aes_decrypt( unsigned char* ciphertext, int ciphertext_len, unsigned char* key,
             unsigned char* iv, unsigned char* plaintext )
{
    EVP_CIPHER_CTX* ctx;

    int len;

    int plaintext_len;

    // Create and initialize the context
    if( !(ctx = EVP_CIPHER_CTX_new()) )
        handleErrors();

    // Initialize decryption
    if( 1 != EVP_DecryptInit_ex( ctx, EVP_aes_128_cbc(), NULL, key, iv ) )
        handleErrors();

    // Decrypt
    if( 1 != EVP_DecryptUpdate( ctx, plaintext, &len, ciphertext, ciphertext_len ) )
        handleErrors();
    plaintext_len = len;

    // Finish decryption
    if( 1 != EVP_DecryptFinal_ex( ctx, plaintext + len, &len ) )
        handleErrors();
    plaintext_len += len;

    // Free context
    EVP_CIPHER_CTX_free( ctx );

    return plaintext_len;
}