package configutils // import "github.com/fractal/fractal/host-service/mandelbox/configutils"

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/sha256" // getConfigTokenHash returns a hash of the config token.

	"github.com/fractal/fractal/host-service/utils"
	"golang.org/x/crypto/pbkdf2"
)

const (
	aes256KeyLength = 32
	aes256IVLength  = 16

	// This is the default iteration value used by openssl -pbkdf2 flag
	pbkdf2Iterations = 10000

	// This is openssl's "magic" salt header value
	opensslSaltHeader = "Salted__"
)

// getSaltAndDataFromOpenSSLEncryptedFile takes OpenSSL encrypted data
// and returns the salt used to encrypt and the encrypted data itself.
func getSaltAndDataFromOpenSSLEncryptedFile(file []byte) (salt, data []byte, err error) {
	if len(file) < aes.BlockSize {
		return nil, nil, utils.MakeError("file is too short to have been encrypted by openssl")
	}

	// All OpenSSL encrypted files that use a salt have a 16-byte header
	// The first 8 bytes are the string "Salted__"
	// The next 8 bytes are the salt itself
	// See: https://security.stackexchange.com/questions/29106/openssl-recover-key-and-iv-by-passphrase
	saltHeader := file[:aes.BlockSize]
	byteHeaderLength := len([]byte(opensslSaltHeader))
	if string(saltHeader[:byteHeaderLength]) != opensslSaltHeader {
		return nil, nil, utils.MakeError("file does not appear to have been encrypted by openssl: wrong salt header")
	}

	salt = saltHeader[byteHeaderLength:]
	data = file[aes.BlockSize:]

	return
}

// getKeyAndIVFromPassAndSalt uses pbkdf2 to generate a key, IV pair
// given a known password and salt.
func getKeyAndIVFromPassAndSalt(password, salt []byte) (key, iv []byte) {
	derivedKey := pbkdf2.Key(password, salt, pbkdf2Iterations, aes256KeyLength+aes256IVLength, sha256.New)

	// First 32 bytes of the derived key are the key and the next 16 bytes are the IV
	key = derivedKey[:aes256KeyLength]
	iv = derivedKey[aes256KeyLength : aes256KeyLength+aes256IVLength]

	return
}

// decryptAES256CBC takes a key, IV, and AES-256 CBC encrypted data and decrypts
// the data in place.
// Based on example code from: https://golang.org/src/crypto/cipher/example_test.go.
func decryptAES256CBC(key, iv, data []byte) error {
	block, err := aes.NewCipher(key)
	if err != nil {
		return utils.MakeError("could not create aes cipher block: %v", err)
	}

	// Validate that encrypted data follows the correct format
	// This is because CBC encryption always works in whole blocks
	if len(data)%aes.BlockSize != 0 {
		return utils.MakeError("given data is not a multiple of the block size")
	}

	mode := cipher.NewCBCDecrypter(block, iv)
	mode.CryptBlocks(data, data)

	// Remove PKCS7 padding
	data, err = unpadPKCS7(data)
	if err != nil {
		return utils.MakeError("could not remove PKCS7 padding: %v", err)
	}

	return nil
}

// unpadPKCS7 takes AES-256 CBC decrypted data and undoes
// the PKCS#7 padding that was applied before encryption.
// See: https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS#5_and_PKCS#7
func unpadPKCS7(data []byte) ([]byte, error) {
	if len(data) < 1 {
		return nil, utils.MakeError("no data to unpad")
	}

	// The last byte of the data will always be the number of bytes to unpad
	padLength := int(data[len(data)-1])

	// Validate the padding length is valid
	if padLength > len(data) || padLength > aes.BlockSize || padLength < 1 {
		return nil, utils.MakeError("invalid padding length of %d is longer than data size (%d), AES block length (%d), or less than one", padLength, len(data), aes.BlockSize)
	}

	// Validate each byte of the padding to check correctness
	for _, v := range data[len(data)-padLength:] {
		if int(v) != padLength {
			return nil, utils.MakeError("invalid padding byte %d, expected %d", v, padLength)
		}
	}

	// Return data minus padding
	return data[:len(data)-padLength], nil
}
