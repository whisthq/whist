#include "aes.h"

#include "openssl/conf.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "openssl/hmac.h"

void handleErrors( void )
{
    ERR_print_errors_fp( stderr );
    abort();
}

void gen_iv( unsigned char* iv )
{
    int rc = RAND_bytes( iv, 16 );
}

void hmac( char* hash, char* buf, int len, char* key )
{
    HMAC_CTX* c = NULL;

    if( (c = HMAC_CTX_new()) == NULL )
        goto err;

    if( !HMAC_Init_ex( c, key, 16, EVP_sha256(), NULL ) )
        goto err;
    if( !HMAC_Update( c, buf, len ) )
        goto err;
    if( !HMAC_Final( c, hash, 16 ) )
        goto err;

    HMAC_CTX_free( c );
    return;

err:
    HMAC_CTX_free( c );
    return;
}

int encrypt_packet( struct RTPPacket* plaintext_packet, int packet_len, struct RTPPacket* encrypted_packet, unsigned char* private_key)
{
    int crypto_header_len = sizeof( plaintext_packet->hash ) + sizeof(plaintext_packet->cipher_len ) +  sizeof( plaintext_packet->iv );

    char* plaintext_buf = (char*)plaintext_packet + crypto_header_len;
    int plaintext_buf_len = packet_len - crypto_header_len;
    gen_iv( encrypted_packet->iv );

    char* cipher_buf = (char*)encrypted_packet + crypto_header_len;
    int cipher_len = encrypt( plaintext_buf, plaintext_buf_len, private_key, encrypted_packet->iv, cipher_buf );
    encrypted_packet->cipher_len = cipher_len;

    int cipher_packet_len = cipher_len + crypto_header_len;

    hmac( encrypted_packet->hash, (char*)encrypted_packet + sizeof( encrypted_packet->hash ), cipher_packet_len - sizeof( encrypted_packet->hash ), PRIVATE_KEY );
    //encrypted_packet->hash = Hash( (char*)encrypted_packet + sizeof( encrypted_packet->hash ), cipher_packet_len - sizeof( encrypted_packet->hash ) );

    return cipher_packet_len;
}

int decrypt_packet( struct RTPPacket* encrypted_packet, int packet_len, struct RTPPacket* plaintext_packet, unsigned char* private_key )
{
    int crypto_header_len = sizeof( plaintext_packet->hash ) + sizeof( plaintext_packet->cipher_len ) +  sizeof( plaintext_packet->iv );

    char hash[16];
    hmac( hash, (char*)encrypted_packet + sizeof( encrypted_packet->hash ), packet_len - sizeof( encrypted_packet->hash ), PRIVATE_KEY );
    //int verify_hash = Hash( (char*)encrypted_packet + sizeof( encrypted_packet->hash ), packet_len - sizeof( encrypted_packet->hash ) );
    //if( encrypted_packet->hash != verify_hash )
    //{
    //    return -1;
    //}
    mprintf( "Hash: %d\n", Hash( hash, 16 ) );
    mprintf( "Hash: %d\n", Hash( encrypted_packet->hash, 16 ) );

    char* cipher_buf = (char*)encrypted_packet + crypto_header_len;
    char* plaintext_buf = (char*)plaintext_packet + crypto_header_len;

    int decrypt_len = decrypt( cipher_buf, encrypted_packet->cipher_len, private_key,
                               encrypted_packet->iv, plaintext_buf );

    return decrypt_len + crypto_header_len;
}

int encrypt( unsigned char* plaintext, int plaintext_len, unsigned char* key,
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


int decrypt( unsigned char* ciphertext, int ciphertext_len, unsigned char* key,
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