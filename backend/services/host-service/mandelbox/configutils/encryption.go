package configutils // import "github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"io"

	"github.com/whisthq/whist/backend/services/utils"
	"golang.org/x/crypto/argon2"
)

const (
	aes256KeyLength      = 32
	aes256GCMNonceLength = 12
	defaultSaltLength    = 16

	saltHeader  = "salt_"
	nonceHeader = "nonce_"
)

// EncryptAES256GCM takes a password and plaintext data and encrypts the data using GCM.
// AES-GCM is an AEAD (authenticated encryption with associated data) method that
// no only provides data confidentiality but also a way to verify authenticity/integrity
// of the data. GCM is also highly performant and fully-parallelizable.
//
// See: https://en.wikipedia.org/wiki/Galois/Counter_Mode
// Based on example code from: https://golang.org/src/crypto/cipher/example_test.go
func EncryptAES256GCM(password string, data []byte) ([]byte, error) {
	// Generate a random salt to use for key generation
	salt, err := generateRandomBytes(defaultSaltLength)
	if err != nil {
		return nil, utils.MakeError("could not generate salt: %s", err)
	}

	// We need to generate a cryptographically secure key from the password
	key := generateKeyFromPasswordAndSalt(password, salt, aes256KeyLength)

	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, utils.MakeError("could not create aes cipher block: %s", err)
	}

	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, utils.MakeError("could not create gcm cipher mode: %s", err)
	}

	// Generate a random nonce to use for encryption
	nonce, err := generateRandomBytes(gcm.NonceSize())
	if err != nil {
		return nil, utils.MakeError("could not generate nonce: %s", err)
	}

	// We want to store the salt and nonce as a prefix to the encrypted data
	prefix := append([]byte(saltHeader), salt...)
	prefix = append(prefix, []byte(nonceHeader)...)
	prefix = append(prefix, nonce...)

	// Encrypt the data and append it to the prefix
	encryptedData := gcm.Seal(prefix, nonce, data, nil)
	return encryptedData, nil
}

// DecryptAES256GCM takes a password and AES256 GCM encrypted data
// and returns the decrypted data or an error.
//
// Based on example code from: https://golang.org/src/crypto/cipher/example_test.go
func DecryptAES256GCM(password string, data []byte) ([]byte, error) {
	// Extract the salt and nonce from the encrypted data
	salt, nonce, encryptedData, err := getSaltNonceAndDataFromEncryptedFile(data, defaultSaltLength, aes256GCMNonceLength)
	if err != nil {
		return nil, utils.MakeError("could not extract salt and nonce from encrypted data: %s", err)
	}

	// Generate the encryption key used from the password and salt
	key := generateKeyFromPasswordAndSalt(password, salt, aes256KeyLength)

	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, utils.MakeError("could not create aes cipher block: %s", err)
	}

	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, utils.MakeError("could not create gcm cipher mode: %s", err)
	}

	decryptedData, err := gcm.Open(nil, nonce, encryptedData, nil)
	if err != nil {
		return nil, utils.MakeError("could not decrypt data: %s", err)
	}

	return decryptedData, nil
}

// getSaltNonceAndDataFromEncryptedFile takes an encrypted file and
// attempts to extract the salt and nonce used to encrypt the file.
func getSaltNonceAndDataFromEncryptedFile(file []byte, saltLength, nonceLength int) (salt, nonce, data []byte, err error) {
	// The salt and nonce should be prefixed to the data
	saltHeaderLength := len(saltHeader)
	nonceHeaderLength := len(nonceHeader)

	totalSaltLength := saltLength + saltHeaderLength
	totalNonceLength := nonceLength + nonceHeaderLength

	prefixLength := totalSaltLength + totalNonceLength

	if len(file) < prefixLength {
		return nil, nil, nil, utils.MakeError("encrypted file is too short")
	}

	// Verify salt header and extract salt
	extractedSaltHeader := string(file[:saltHeaderLength])
	if extractedSaltHeader != saltHeader {
		return nil, nil, nil, utils.MakeError("encrypted file does not have a valid salt header")
	}
	salt = file[saltHeaderLength:totalSaltLength]

	// Verify nonce header and extract nonce
	extractedNonceHeader := string(file[totalSaltLength : totalSaltLength+nonceHeaderLength])
	if extractedNonceHeader != nonceHeader {
		return nil, nil, nil, utils.MakeError("encrypted file does not have a valid nonce header")
	}
	nonce = file[totalSaltLength+nonceHeaderLength : prefixLength]

	// The remaining is encrypted data
	data = file[prefixLength:]
	return
}

// generateKeyFromPasswordAndSalt takes a password and salt and returns
// a cryptographically secure key using argon2id with default parameters.
func generateKeyFromPasswordAndSalt(password string, salt []byte, length uint32) []byte {
	return argon2.IDKey([]byte(password), salt, 1, 64*1024, 8, length)
}

// generateRandomBytes takes a length and returns a slice of random bytes of that length.
func generateRandomBytes(length int) ([]byte, error) {
	randomData := make([]byte, length)
	if _, err := io.ReadFull(rand.Reader, randomData); err != nil {
		return nil, utils.MakeError("could not generate random bytes: %s", err)
	}
	return randomData, nil
}
