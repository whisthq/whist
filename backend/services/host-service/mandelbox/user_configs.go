package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"bytes"
	"crypto/md5"
	"encoding/base32"
	"errors"
	"os"
	"os/exec"
	"path"

	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	s3types "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/aws/smithy-go"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	types "github.com/whisthq/whist/backend/services/host-service/mandelbox/types"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// Exported members

const (
	// UnpackedConfigsDirectoryName is the name of the directory in the mandelbox that stores
	// unpacked user configs.
	UnpackedConfigsDirectoryName = "unpacked_configs/"

	// EncryptedArchiveFilename is the name of the encrypted user config file.
	EncryptedArchiveFilename = "fractal-app-config.tar.lz4.enc"
)

// StartLoadingUserConfigs starts the process of loading user configs. It returns
// immediately, providing a channel used to send the configEncryptionToken once
// it's available (to start the decryption process), and a channel that returns
// errors with the process as they occur. The configEncryptionToken channel
// should only ever have exactly one value sent over it, but the error channel
// might potentially have multiple errors to report. The error channel will
// close to signify the completion of loading user configs. This design allows
// the config decryption to occur in parallel with the other steps to spinning
// up a mandelbox, while also providing the synchronization necessary.
func (mandelbox *mandelboxData) StartLoadingUserConfigs() (chan<- types.ConfigEncryptionToken, <-chan error) {
	// We buffer both channels to prevent blocking.
	configEncryptionTokenChan := make(chan types.ConfigEncryptionToken, 1)
	errorChan := make(chan error, 10)

	// This inner function is where we perform all the real work of loading user configs.
	go mandelbox.loadUserConfigs(configEncryptionTokenChan, errorChan)

	return configEncryptionTokenChan, errorChan
}

// loadUserConfigs does the "real work" of LoadUserConfigs.
func (mandelbox *mandelboxData) loadUserConfigs(tokenChan <-chan types.ConfigEncryptionToken, errorChan chan<- error) {
	defer close(errorChan)

	// If userID is not set, then we don't retrieve configs from s3
	if len(mandelbox.GetUserID()) == 0 {
		errorChan <- utils.MakeError("User ID is not set for mandelbox %s. Skipping config download.", mandelbox.GetID())
		return
	}

	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		errorChan <- utils.MakeError("Could not make new S3 client: %s", err)
		return
	}

	// At this point, we are still waiting on the client-app to get the user's
	// config encryption token. Instead of waiting, we want to speculatively
	// download a config that we think is the most likely to be the one we need.
	_, err = mandelbox.predictConfigToDownload(s3Client)
	if err != nil {
		errorChan <- utils.MakeError("There are no existing configs in s3 for user %s", mandelbox.GetUserID())
		return
	}

	// TODO: More logic here
}

// predictConfigToDownload guesses which config is the most likely to be the
// one we need to download, given the user's config encryption token.
func (mandelbox *mandelboxData) predictConfigToDownload(s3client *s3.Client) (*s3types.Object, error) {
	// We use the simple guess that the most recently-modified config is the one
	// that we need.

	prefix := path.Join(string(mandelbox.GetUserID()), metadata.GetAppEnvironmentLowercase(), string(mandelbox.GetAppName()))
	return configutils.GetMostRecentMatchingKey(s3client, userConfigS3Bucket, prefix, EncryptedArchiveFilename)
}

// Helpers

const (
	userConfigS3Bucket = "fractal-user-app-configs"
	aes256KeyLength    = 32
	aes256IVLength     = 16
)

// DownloadUserConfigs downloads user configs from S3 and saves them to an in-memory buffer.
func (mandelbox *mandelboxData) DownloadUserConfigs() error {
	s3ConfigKey := mandelbox.getS3ConfigKey()

	logger.Infof("Starting S3 config download for mandelbox %s", mandelbox.GetID())

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
			logger.Infof("Could not get head object because config does not exist for user %s", mandelbox.GetUserID())
			return nil
		}

		return utils.MakeError("Failed to download head object from s3: %v", err)
	}

	// Log config version
	logger.Infof("Using user config version %s for mandelbox %s", *headObject.VersionId, mandelbox.GetID())

	// Download file into a pre-allocated in-memory buffer
	// This should be okay as we don't expect configs to be very large
	configBuffer := manager.NewWriteAtBuffer(make([]byte, headObject.ContentLength))
	numBytes, err := configutils.DownloadObjectToBuffer(s3Client, userConfigS3Bucket, s3ConfigKey, configBuffer)

	var noSuchKeyErr *s3types.NoSuchKey
	if err != nil {
		if errors.As(err, &noSuchKeyErr) {
			logger.Infof("Could not download user config because config does not exist for user %s", mandelbox.GetUserID())
			return nil
		}

		return utils.MakeError("Failed to download user configuration from s3: %v", err)
	}

	mandelbox.SetConfigBuffer(configBuffer)
	logger.Infof("Downloaded %d bytes from s3 for mandelbox %s", numBytes, mandelbox.GetID())

	return nil
}

// DecryptUserConfigs decrypts and unpacks the previously downloaded
// s3 config using the encryption token received through JSON transport.
func (mandelbox *mandelboxData) DecryptUserConfigs() error {
	if len(mandelbox.GetConfigEncryptionToken()) == 0 {
		return utils.MakeError("Cannot get user configs for MandelboxID %s since ConfigEncryptionToken is empty", mandelbox.GetID())
	}

	if mandelbox.GetConfigBuffer() == nil {
		logger.Infof("Not decrypting user configs because the config buffer is empty.")
		return nil
	}

	logger.Infof("Decrypting user config for mandelbox %s", mandelbox.GetID())
	logger.Infof("Using (hashed) decryption token %s for mandelbox %s", getTokenHash(string(mandelbox.GetConfigEncryptionToken())), mandelbox.GetID())

	// Decrypt the downloaded archive directly from memory
	encryptedFile := mandelbox.GetConfigBuffer().Bytes()
	decryptedData, err := configutils.DecryptAES256GCM(string(mandelbox.GetConfigEncryptionToken()), encryptedFile)
	if err != nil {
		return utils.MakeError("Failed to decrypt user configs: %v", err)
	}

	logger.Infof("Finished decrypting user config for mandelbox %s", mandelbox.GetID())
	logger.Infof("Decompressing user config for mandelbox %s", mandelbox.GetID())

	// Make directory for user configs
	configDir := mandelbox.GetUserConfigDir()
	unpackedConfigDir := path.Join(configDir, UnpackedConfigsDirectoryName)
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

	// Set the mandelbox config buffer to nil after we're done
	mandelbox.SetConfigBuffer(nil)

	return nil
}

// BackupUserConfigs compresses, encrypts, and then uploads user config files to S3.
func (mandelbox *mandelboxData) BackupUserConfigs() error {
	userID := mandelbox.GetUserID()
	configToken := mandelbox.GetConfigEncryptionToken()
	mandelboxID := mandelbox.GetID()

	if len(userID) == 0 {
		logger.Infof("Cannot save user configs for MandelboxID %s since UserID is empty.", mandelboxID)
		return nil
	}

	if len(configToken) == 0 {
		return utils.MakeError("Cannot save user configs for MandelboxID %s since ConfigEncryptionToken is empty", mandelboxID)
	}

	configDir := mandelbox.GetUserConfigDir()
	unpackedConfigPath := path.Join(configDir, UnpackedConfigsDirectoryName)

	// Compress the user configs into a tar.lz4 file
	compressedConfig, err := configutils.CompressTarLz4(unpackedConfigPath)
	if err != nil {
		return utils.MakeError("Failed to compress user configs: %v", err)
	}

	// Encrypt the compressed config
	logger.Infof("Using (hashed) encryption token %s for mandelbox %s", getTokenHash(string(configToken)), mandelboxID)
	encryptedConfig, err := configutils.EncryptAES256GCM(string(configToken), compressedConfig)
	if err != nil {
		return utils.MakeError("Failed to encrypt user configs: %v", err)
	}
	encryptedConfigBuffer := bytes.NewBuffer(encryptedConfig)

	// Upload encrypted config to S3
	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		return utils.MakeError("error creating s3 client: %v", err)
	}

	uploadResult, err := configutils.UploadFileToBucket(s3Client, userConfigS3Bucket, mandelbox.getS3ConfigKey(), encryptedConfigBuffer)
	if err != nil {
		return utils.MakeError("error uploading encrypted config to s3: %v", err)
	}

	logger.Infof("Saved user config for mandelbox %s, version %s", mandelboxID, *uploadResult.VersionID)

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
	logger.Infof("Creating user config directories for mandelbox %s", mandelbox.GetID())

	configDir := mandelbox.GetUserConfigDir()
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", configDir, err)
	}

	unpackedConfigDir := path.Join(configDir, UnpackedConfigsDirectoryName)
	if err := os.MkdirAll(unpackedConfigDir, 0777); err != nil {
		return utils.MakeError("Could not make dir %s. Error: %s", unpackedConfigDir, err)
	}

	return nil
}

// cleanUserConfigDir removes all user config related files and directories from the host.
func (mandelbox *mandelboxData) cleanUserConfigDir() {
	err := os.RemoveAll(mandelbox.GetUserConfigDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", mandelbox.GetUserConfigDir(), err)
	}
}

// GetUserConfigDir returns the absolute path to the user config directory.
func (mandelbox *mandelboxData) GetUserConfigDir() string {
	return utils.Sprintf("%s%v/%s", utils.WhistDir, mandelbox.GetID(), "userConfigs")
}

// getS3ConfigKey returns the S3 key to the encrypted user config file.
func (mandelbox *mandelboxData) getS3ConfigKey() string {
	return path.Join(string(mandelbox.GetUserID()), metadata.GetAppEnvironmentLowercase(), string(mandelbox.GetAppName()), EncryptedArchiveFilename)
}

// getTokenHash returns a hash of the given token.
func getTokenHash(token string) string {
	hash := md5.Sum([]byte(token))
	return base32.StdEncoding.EncodeToString(hash[:])
}
