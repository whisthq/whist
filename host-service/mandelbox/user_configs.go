package mandelbox // import "github.com/fractal/fractal/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"bytes"
	"crypto/sha256"
	"encoding/base64"
	"errors"
	"os"
	"os/exec"
	"path"

	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/aws/smithy-go"
	"github.com/fractal/fractal/host-service/mandelbox/configutils"
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/utils"
	logger "github.com/fractal/fractal/host-service/whistlogger"
)

const (
	userConfigS3Bucket = "fractal-user-app-configs"
	aes256KeyLength    = 32
	aes256IVLength     = 16
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

	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		return utils.MakeError("failed to create s3 client: %v", err)
	}

	// Fetch the HeadObject first to see how much memory we need to allocate
	logger.Infof("Fetching head object for bucket: %s, key: %s", userConfigS3Bucket, s3ConfigKey)
	headObject, err := configutils.GetHeadObject(s3Client, userConfigS3Bucket, s3ConfigKey)

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
	numBytes, err := configutils.DownloadObjectToBuffer(s3Client, userConfigS3Bucket, s3ConfigKey, mandelbox.configBuffer)

	var noSuchKeyErr *types.NoSuchKey
	if err != nil {
		if errors.As(err, &noSuchKeyErr) {
			logger.Infof("Could not download user config because config does not exist for user %s", mandelbox.userID)
			return nil
		}

		return utils.MakeError("Failed to download user configuration from s3: %v", err)
	}

	logger.Infof("Downloaded %d bytes from s3 for mandelbox %s", numBytes, mandelbox.ID)

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

	// Decrypt the downloaded archive directly from memory
	encryptedFile := mandelbox.configBuffer.Bytes()
	decryptedData, err := configutils.DecryptAES256GCM(string(mandelbox.configEncryptionToken), encryptedFile)
	if err != nil {
		return utils.MakeError("Failed to decrypt user configs: %v", err)
	}

	logger.Infof("Finished decrypting user config for mandelbox %s", mandelbox.ID)
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

	totalFileSize, err := configutils.ExtractTarLz4(decryptedData, unpackedConfigDir)
	if err != nil {
		logger.Errorf("Error extracting tar lz4 file: %v", err)
	}

	logger.Infof("Untarred config to: %s, total size was %d bytes", unpackedConfigDir, totalFileSize)

	return nil
}

// BackupUserConfigs compresses, encrypts, and then uploads user config files to S3.
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
	unpackedConfigPath := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())

	// Compress the user configs into a tar.lz4 file
	compressedConfig, err := configutils.CompressTarLz4(unpackedConfigPath)
	if err != nil {
		return utils.MakeError("Failed to compress user configs: %v", err)
	}

	// Encrypt the compressed config
	logger.Infof("Using (hashed) encryption token %s for mandelbox %s", getTokenHash(string(mandelbox.configEncryptionToken)), mandelbox.ID)
	encryptedConfig, err := configutils.EncryptAES256GCM(string(mandelbox.configEncryptionToken), compressedConfig)
	if err != nil {
		return utils.MakeError("Failed to encrypt user configs: %v", err)
	}
	encryptedConfigBuffer := bytes.NewBuffer(encryptedConfig)

	// Upload encrypted config to S3
	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		return utils.MakeError("error creating s3 client: %v", err)
	}

	uploadResult, err := configutils.UploadFileToBucket(s3Client, userConfigS3Bucket, mandelbox.getS3ConfigKeyWithoutLocking(), encryptedConfigBuffer)
	if err != nil {
		return utils.MakeError("error uploading encrypted config to s3: %v", err)
	}

	logger.Infof("Saved user config for mandelbox %s, version %s", mandelbox.ID, *uploadResult.VersionID)

	return nil
}

// WriteUserInitialBrowserData writes the user's initial browser data to file(s)
// received through JSON transport for later use in the mandelbox
func (mandelbox *mandelboxData) WriteUserInitialBrowserData(cookieJSON string) error {
	// Avoid doing work for empty string/array string
	if len(cookieJSON) <= 2 {
		logger.Infof("Not writing to file as user initial browser data is empty.")
		// the browser data can be empty
		return nil
	}

	// The initial browser cookies will use the same directory as the user config
	// Make directories for user configs if not exists
	configDir := mandelbox.getUserConfigDir()
	if _, err := os.Stat(configDir); os.IsNotExist(err) {
		if err := os.MkdirAll(configDir, 0777); err != nil {
			return utils.MakeError("Could not make dir %s. Error: %s", configDir, err)
		}
		defer func() {
			cmd := exec.Command("chown", "-R", "ubuntu", configDir)
			cmd.Run()
			cmd = exec.Command("chmod", "-R", "777", configDir)
			cmd.Run()
		}()
	}

	unpackedConfigDir := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())
	if _, err := os.Stat(unpackedConfigDir); os.IsNotExist(err) {
		if err := os.MkdirAll(unpackedConfigDir, 0777); err != nil {
			return utils.MakeError("Could not make dir %s. Error: %s", unpackedConfigDir, err)
		}
	}

	// Begin writing user initial browser data
	cookieFilePath := path.Join(unpackedConfigDir, utils.UserInitialCookiesFile)

	cookieFile, err := os.OpenFile(cookieFilePath, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0777)
	if err != nil {
		return utils.MakeError("Could not make file %s. Error: %s", cookieFilePath, err)
	}

	// We want to pass the entire JSON string into the file
	cookieFile.WriteString(cookieJSON)

	logger.Infof("Finished storing user initial browser data.")

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
	logger.Infof("Creating user config directories for mandelbox %s", mandelbox.ID)

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
