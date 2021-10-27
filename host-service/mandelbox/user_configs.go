package mandelbox // import "github.com/fractal/fractal/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"archive/tar"
	"bytes"
	"context"
	"crypto/aes"
	"crypto/cipher"
	"crypto/sha256"
	"encoding/base64"
	"errors"
	"io"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strings"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	awsTypes "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/aws/smithy-go"
	logger "github.com/fractal/fractal/host-service/fractallogger"
	types "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/utils"
	"github.com/pierrec/lz4/v4"
	"golang.org/x/crypto/pbkdf2"
)

const (
	userConfigS3Bucket = "fractal-user-app-configs"
	aes256KeyLength    = 32
	aes256IVLength     = 16

	// This is the default iteration value used by openssl -pbkdf2 flag
	pbkdf2Iterations = 10000

	// This is openssl's "magic" salt header value
	opensslSaltHeader = "Salted__"
)

// DownloadUserConfigs downloads user configs from S3 and saves them to an in-memory buffer.
func (mandelbox *mandelboxData) DownloadUserConfigs() error {
	// If userID is not set, then we don't retrieve configs from s3
	if len(mandelbox.GetUserID()) == 0 {
		logger.Warningf("User ID is not set for mandelbox %s. Skipping config download.", mandelbox.ID)
		return nil
	}

	s3ConfigKey := mandelbox.getS3ConfigKey()

	logger.Infof("Starting S3 config download for mandelbox %s", mandelbox.ID)

	cfg, err := config.LoadDefaultConfig(context.Background())
	if err != nil {
		return utils.MakeError("failed to load aws config: %v", err)
	}

	s3Client := s3.NewFromConfig(cfg, func(o *s3.Options) {
		o.Region = "us-east-1"
	})
	downloader := manager.NewDownloader(s3Client)
	logger.Infof("Fetching head object for bucket: %s, key: %s", userConfigS3Bucket, s3ConfigKey)
	// Fetch the HeadObject first to see how much memory we need to allocate
	headObject, err := s3Client.HeadObject(context.Background(), &s3.HeadObjectInput{
		Bucket: aws.String(userConfigS3Bucket),
		Key:    aws.String(s3ConfigKey),
	})

	var apiErr smithy.APIError
	if err != nil {
		// If aws s3 errors out due to the file not existing, don't log an error because
		// this means that it's the user's first run and they don't have any settings
		// stored for this application yet.
		if errors.As(err, &apiErr) && apiErr.ErrorCode() == "NotFound" {
			logger.Infof("Could not get head object because config does not exist for user %s", mandelbox.userID)
			return nil
		}

		return utils.MakeError("Failed to download head object from s3: %v", err)
	}

	// Log config version
	logger.Infof("Using user config version %s for mandelbox %s", *headObject.VersionId, mandelbox.ID)

	// Download file into a pre-allocated in-memory buffer
	// This should be okay as we don't expect configs to be very large
	mandelbox.configBuffer = manager.NewWriteAtBuffer(make([]byte, headObject.ContentLength))
	numBytes, err := downloader.Download(context.Background(), mandelbox.configBuffer, &s3.GetObjectInput{
		Bucket: aws.String(userConfigS3Bucket),
		Key:    aws.String(s3ConfigKey),
	})

	var noSuchKeyErr *types.NoSuchKey
	if err != nil {
		return err
	}

	return nil
}

// DecryptUserConfigs decrypts and unpacks the previously downloaded
// s3 config using the encryption token received through JSON transport.
func (mandelbox *mandelboxData) DecryptUserConfigs() error {

	if len(mandelbox.configEncryptionToken) == 0 {
		return utils.MakeError("Cannot get user configs for MandelboxID %s since ConfigEncryptionToken is empty", mandelbox.ID)
	}

	if mandelbox.configBuffer == nil {
		logger.Infof("Not decrypting user configs because the config buffer is empty.")
		return nil
	}

	logger.Infof("Decrypting user config for mandelbox %s", mandelbox.ID)
	logger.Infof("Using (hashed) decryption token %s for mandelbox %s", getTokenHash(string(mandelbox.GetConfigEncryptionToken())), mandelbox.ID)

	var data []byte
	err := c.decryptFileInMemory(data)

	if err != nil {
		return err
	} else if len(data) == 0 {
		return nil
	}

	logger.Infof("Decompressing user config for mandelbox %s", mandelbox.ID)

	// Lock directory to avoid cleanup
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	// Make directory for user configs
	configDir := mandelbox.getUserConfigDir()
	unpackedConfigDir := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())
	err = mandelbox.SetupUserConfigDirs()
	if err != nil {
		return utils.MakeError("failed to setup user config directories: %v", err)
	}

	// Once we've extracted everything, we open up permissions for the user
	// config directory so it's accessible by the non-root user in the
	// mandelboxes. We also are okay with setting executable bits here, since
	// it's possible that some apps store executable scripts in their config.
	defer func() {
		cmd := exec.Command("chown", "-R", "ubuntu", configDir)
		cmd.Run()
		cmd = exec.Command("chmod", "-R", "777", configDir)
		cmd.Run()
	}()

	err = untarFiles(data, unpackedConfigDir)

	if err != nil {
		return err
	}

	return nil
}

// backupUserConfigs compresses, encrypts, and then uploads user config files to S3.
// Requires `mandelbox.rwlock` to be locked.
func (mandelbox *mandelboxData) BackupUserConfigs() error {
	if len(mandelbox.userID) == 0 {
		logger.Infof("Cannot save user configs for MandelboxID %s since UserID is empty.", mandelbox.ID)
		return nil
	}

	if len(mandelbox.configEncryptionToken) == 0 {
		return utils.MakeError("Cannot save user configs for MandelboxID %s since ConfigEncryptionToken is empty", mandelbox.ID)
	}

	configDir := mandelbox.getUserConfigDir()
	encTarPath := path.Join(configDir, mandelbox.getEncryptedArchiveFilename())
	decTarPath := path.Join(configDir, mandelbox.getDecryptedArchiveFilename())
	unpackedConfigPath := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())

	err := tarFile(unpackedConfigPath, decTarPath)
	if err != nil {
		return err
	}

	// At this point, config archive must exist: encrypt app config
	logger.Infof("Using (hashed) encryption token %s for mandelbox %s", getTokenHash(string(mandelbox.configEncryptionToken)), mandelbox.ID)
	encryptConfigCmd := exec.Command(
		"/usr/bin/openssl", "aes-256-cbc", "-e",
		"-in", decTarPath,
		"-out", encTarPath,
		"-pass", "pass:"+string(mandelbox.configEncryptionToken), "-pbkdf2")
	encryptConfigOutput, err := encryptConfigCmd.CombinedOutput()
	if err != nil {
		return err
	}

	err = mandelbox.uploadAndEncryptToS3(mandelbox.getS3ConfigKey(), encTarPath)

	if err != nil {
		return err
	}
	logger.Infof("Saved user config for mandelbox %s", mandelbox.mandelboxID)

	return nil
}

// WriteJSONData writes the data received through JSON transport
// to the config.json file located on the resourceMappingDir.
func (mandelbox *mandelboxData) WriteJSONData() error {
	logger.Infof("Writing JSON transport data to config.json file...")

	if err := mandelbox.writeResourceMappingToFile("config.json", mandelbox.GetJSONData()); err != nil {
		return err
	}

	return nil
}

// SetupUserConfigDirs creates the user config directories on the host.
func (mandelbox *mandelboxData) SetupUserConfigDirs() error {
	logger.Infof("Creating user config directories...")

	configDir := mandelbox.getUserConfigDir()
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", configDir, err)
	}

	unpackedConfigDir := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())
	if err := os.MkdirAll(unpackedConfigDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", unpackedConfigDir, err)
	}

	return nil
}

// cleanUserConfigDir removes all user config related files and directories from the host.
// Requires `mandelbox.rwlock` to be locked.
func (mandelbox *mandelboxData) cleanUserConfigDir() {
	err := os.RemoveAll(mandelbox.getUserConfigDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", mandelbox.getUserConfigDir(), err)
	}
}

// getUserConfigDir returns the absolute path to the user config directory.
func (mandelbox *mandelboxData) getUserConfigDir() string {
	return utils.Sprintf("%s%v/%s", utils.FractalDir, mandelbox.GetID(), "userConfigs")
}

// getS3ConfigKey returns the S3 key to the encrypted user config file.
func (mandelbox *mandelboxData) getS3ConfigKey() string {
	return path.Join(string(mandelbox.GetUserID()), metadata.GetAppEnvironmentLowercase(), string(mandelbox.GetAppName()), mandelbox.getEncryptedArchiveFilename())
}

// getS3ConfigKeyWithoutLocking returns the S3 key to the encrypted user config file
// without locking the mandelbox. This is used when the enclosing function already
// holds a lock.
func (mandelbox *mandelboxData) getS3ConfigKeyWithoutLocking() string {
	return path.Join(string(mandelbox.userID), metadata.GetAppEnvironmentLowercase(), string(mandelbox.appName), mandelbox.getEncryptedArchiveFilename())
}

// getEncryptedArchiveFilename returns the name of the encrypted user config file.
func (mandelbox *mandelboxData) getEncryptedArchiveFilename() string {
	return "fractal-app-config.tar.lz4.enc"
}

// getDecryptedArchiveConfigFilename returns the name of the
// decrypted (but still compressed) user config file.
func (mandelbox *mandelboxData) getDecryptedArchiveFilename() string {
	return "fractal-app-config.tar.lz4"
}

// getEncryptedArchiveCustomConfigFilename returns the name of the encrypted user custom config file(s).
func getEncryptedArchiveCustomConfigFilename() string {
	return "fractal-app-custom-config.tar.lz4.enc"
}

// getCookieFilename returns the name of the cookie file
func getCookieFilename() string {
	return "fractal-app-config-cookies"
}


// getUnpackedConfigsDirectoryName returns the name of the
// directory that stores unpacked user configs.
func (mandelbox *mandelboxData) getUnpackedConfigsDirectoryName() string {
	return "unpacked_configs/"
}

// getTokenHash returns a hash of the given token.
func getTokenHash(token string) string {
	hash := sha256.Sum256([]byte(token))
	return base64.StdEncoding.EncodeToString(hash[:])
}

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
