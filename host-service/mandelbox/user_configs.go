package mandelbox // import "github.com/fractal/fractal/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/sha256"
	"os"
	"os/exec"
	"strings"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"
	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/utils"
	"golang.org/x/crypto/pbkdf2"
)

const (
	USER_CONFIG_S3_BUCKET = "fractal-user-app-configs"
	AES_256_KEY_LENGTH    = 32
	AES_256_IV_LENGTH     = 16
	PBKDF2_ITERATIONS     = 10000
	OPENSSL_SALT_HEADER   = "Salted__"
)

func (c *mandelboxData) PopulateUserConfigs() error {
	// We (write) lock here so that the resourceMappingDir doesn't get cleaned up
	// from under us.
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	// Make directory for user configs
	configDir := c.getUserConfigDir()
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", configDir, err)
	}

	unpackedConfigDir := configDir + c.getUnpackedConfigsDirectoryName()
	if err := os.MkdirAll(unpackedConfigDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", unpackedConfigDir, err)
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

	// If userID is not set, then we don't retrieve configs from s3
	if len(c.userID) == 0 {
		return nil
	}

	if len(c.configEncryptionToken) == 0 {
		return utils.MakeError("Cannot get user configs for MandelboxID %s since ConfigEncryptionToken is empty", c.mandelboxID)
	}

	encTarPath := configDir + c.getEncryptedArchiveFilename()
	decTarPath := configDir + c.getDecryptedArchiveFilename()
	unpackedConfigPath := configDir + c.getUnpackedConfigsDirectoryName()

	logger.Infof("Starting S3 config download")

	sess, err := session.NewSession(&aws.Config{
		Region: aws.String("us-east-1"),
	})
	downloader := s3manager.NewDownloader(sess)

	// Download file into an in-memory buffer
	// This should be okay as we don't expect configs to be very large
	buf := aws.NewWriteAtBuffer([]byte{})

	numBytes, err := downloader.Download(buf, &s3.GetObjectInput{
		Bucket: aws.String(USER_CONFIG_S3_BUCKET),
		Key:    aws.String(c.getS3ConfigKey()),
	})
	if err != nil {
		// If aws s3 cp errors out due to the file not existing, don't log an error because
		// this means that it's the user's first run and they don't have any settings
		// stored for this application yet.
		if awsErr, ok := err.(awserr.Error); ok {
			switch awsErr.Code() {
			case s3.ErrCodeNoSuchKey:
				logger.Infof("Ran \"aws s3 cp\" and config does not exist")
				return nil
			}
		}

		return utils.MakeError("Failed to download user configuration from s3: %v", err)
	}

	logger.Infof("Ran \"aws s3 cp\" get config command and received %d bytes", numBytes)

	logger.Infof("Starting decryption")

	// Decrypt the downloaded archive directly from memory
	encryptedFile := buf.Bytes()
	salt, encryptedData, err := getSaltAndDataFromOpenSSLEncryptedFile(encryptedFile)
	if err != nil {
		return utils.MakeError("failed to get salt and data from encrypted file: %v", err)
	}

	key, iv := getKeyAndIVFromPassAndSalt([]byte(c.configEncryptionToken), salt)
	decryptedData, err := decryptAES256CBC(key, iv, encryptedData)
	if err != nil {
		return utils.MakeError("failed to decrypt user config: %v", err)
	}

	logger.Infof("decrypt complete")
	logger.Infof("writing to file: %s", decTarPath)

	err = os.WriteFile(decTarPath, decryptedData, 0777)

	logger.Infof("file write completed")
	logger.Infof("Starting extraction")

	// Extract the config archive to the user config directory
	untarConfigCmd := exec.Command("/usr/bin/tar", "-I", "lz4", "-xf", decTarPath, "-C", unpackedConfigPath)
	untarConfigOutput, err := untarConfigCmd.CombinedOutput()
	if err != nil {
		return utils.MakeError("Could not untar config archive: %s. Output: %s", err, untarConfigOutput)
	}
	logger.Infof("Untar config directory output: %s", untarConfigOutput)

	return nil
}

// This function requires that `c.rwlock()` is locked. It backs up the user
// config to s3.
func (c *mandelboxData) backupUserConfigs() error {
	if len(c.userID) == 0 {
		logger.Infof("Cannot save user configs for MandelboxID %s since UserID is empty.", c.mandelboxID)
		return nil
	}

	if len(c.configEncryptionToken) == 0 {
		return utils.MakeError("Cannot save user configs for MandelboxID %s since ConfigEncryptionToken is empty", c.mandelboxID)
	}

	configDir := c.getUserConfigDir()
	encTarPath := configDir + c.getEncryptedArchiveFilename()
	decTarPath := configDir + c.getDecryptedArchiveFilename()
	unpackedConfigPath := configDir + c.getUnpackedConfigsDirectoryName()
	s3ConfigPath := c.getS3ConfigPath()

	tarConfigCmd := exec.Command(
		"/usr/bin/tar", "-I", "lz4", "-C", unpackedConfigPath, "-cf", decTarPath,
		".")
	tarConfigOutput, err := tarConfigCmd.CombinedOutput()
	// tar is only fatal when exit status is 2 -
	//    exit status 1 just means that some files have changed while tarring,
	//    which is an ignorable error
	if err != nil && !strings.Contains(string(tarConfigOutput), "file changed") {
		return utils.MakeError("Could not tar config directory: %s. Output: %s", err, tarConfigOutput)
	}
	logger.Infof("Tar config directory output: %s", tarConfigOutput)

	// At this point, config archive must exist: encrypt app config
	encryptConfigCmd := exec.Command(
		"/usr/bin/openssl", "aes-256-cbc", "-e",
		"-in", decTarPath,
		"-out", encTarPath,
		"-pass", "pass:"+string(c.configEncryptionToken), "-pbkdf2")
	encryptConfigOutput, err := encryptConfigCmd.CombinedOutput()
	if err != nil {
		// If the config could not be encrypted, don't upload
		return utils.MakeError("Could not encrypt config: %s. Output: %s", err, encryptConfigOutput)
	}
	logger.Infof("Encrypted config to %s", encTarPath)

	saveConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", encTarPath, s3ConfigPath)
	saveConfigOutput, err := saveConfigCmd.CombinedOutput()
	if err != nil {
		return utils.MakeError("Could not run \"aws s3 cp\" save config command: %s. Output: %s", err, saveConfigOutput)
	}
	logger.Infof("Ran \"aws s3 cp\" save config command with output: %s", saveConfigOutput)

	return nil
}

// This function requires that `c.rwlock()` is locked. It cleans up the user
// config directory for a mandelbox.
func (c *mandelboxData) cleanUserConfigDir() {
	err := os.RemoveAll(c.getUserConfigDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", c.getUserConfigDir(), err)
	}
}

func (c *mandelboxData) getUserConfigDir() string {
	return utils.Sprintf("%s%s/userConfigs/", utils.FractalDir, c.mandelboxID)
}

func (c *mandelboxData) getS3ConfigPath() string {
	return utils.Sprintf("s3://fractal-user-app-configs/%s/%s/%s/%s", c.userID, metadata.GetAppEnvironmentLowercase(), c.appName, c.getEncryptedArchiveFilename())
}

func (c *mandelboxData) getS3ConfigKey() string {
	return utils.Sprintf("%s/%s/%s/%s", c.userID, metadata.GetAppEnvironmentLowercase(), c.appName, c.getEncryptedArchiveFilename())
}

func (c *mandelboxData) getEncryptedArchiveFilename() string {
	return "fractal-app-config.tar.lz4.enc"
}

func (c *mandelboxData) getDecryptedArchiveFilename() string {
	return "fractal-app-config.tar.lz4"
}

func (c *mandelboxData) getUnpackedConfigsDirectoryName() string {
	return "unpacked_configs/"
}

// getSaltAndDataFromOpenSSLEncryptedFile takes OpenSSL encrypted data
// and returns the salt used to encrypt and the encrypted data itself
func getSaltAndDataFromOpenSSLEncryptedFile(file []byte) (salt, data []byte, err error) {
	if len(file) < aes.BlockSize {
		return nil, nil, utils.MakeError("file is too short to have been encrypted by openssl")
	}

	// All OpenSSL encrypted files that use a salt have a 16-byte header
	// The first 8 bytes are the string "Salted__"
	// The next 8 bytes are the salt itself
	// See: https://security.stackexchange.com/questions/29106/openssl-recover-key-and-iv-by-passphrase
	saltHeader := file[:aes.BlockSize]
	byteHeaderLength := len([]byte(OPENSSL_SALT_HEADER))
	if string(saltHeader[:byteHeaderLength]) != OPENSSL_SALT_HEADER {
		return nil, nil, utils.MakeError("file does not appear to have been encrypted by openssl: wrong salt header")
	}

	salt = saltHeader[byteHeaderLength:]
	data = file[aes.BlockSize:]

	return
}

// getKeyAndIVFromPassAndSalt uses pbkdf2 to generate a key, IV pair
// given a known password and salt
func getKeyAndIVFromPassAndSalt(password, salt []byte) (key, iv []byte) {
	derivedKey := pbkdf2.Key(password, salt, PBKDF2_ITERATIONS, AES_256_KEY_LENGTH+AES_256_IV_LENGTH, sha256.New)

	// First 32 bytes of the derived key are the key and the next 16 bytes are the IV
	key = derivedKey[:AES_256_KEY_LENGTH]
	iv = derivedKey[AES_256_KEY_LENGTH : AES_256_KEY_LENGTH+AES_256_IV_LENGTH]

	return
}

// decryptAES256CBC takes a key, IV, and AES-256 CBC encrypted data and returns the decrypted data
// Based on example code from: https://golang.org/src/crypto/cipher/example_test.go
func decryptAES256CBC(key, iv, data []byte) (decrypted []byte, err error) {
	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, utils.MakeError("could not create aes cipher block: %v", err)
	}

	// Validate that encrypted data follows the correct format
	// This is because CBC encryption always works in whole blocks
	if len(data)%aes.BlockSize != 0 {
		return nil, utils.MakeError("given data is not a multiple of the block size")
	}

	mode := cipher.NewCBCDecrypter(block, iv)
	mode.CryptBlocks(decrypted, data)

	return
}
