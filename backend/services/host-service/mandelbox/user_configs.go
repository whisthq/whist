package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"bytes"
	"crypto/md5"
	"encoding/base32"
	"os"
	"os/exec"
	"path"

	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	s3types "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/metadata"
	types "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// Exported members

const (
	// UnpackedConfigsDirectoryName is the name of the directory in the mandelbox that stores
	// unpacked user configs.
	UnpackedConfigsDirectoryName = "unpacked_configs/"

	// UserConfigS3Bucket is the name of the S3 bucket that contains the encrypted user configs.
	UserConfigS3Bucket = "fractal-user-app-configs"

	// EncryptedArchiveFilename is the name of the encrypted user config file.
	EncryptedArchiveFilename = "fractal-app-config.tar.lz4.enc"
)

// ConfigEncryptionInfo defines the information we want from the client-app to successfully decrypt configs.
type ConfigEncryptionInfo struct {
	Token types.ConfigEncryptionToken
	// When IsNewTokenAccordingToClientApp is true, then the client app
	// effectively believes that the client-app is starting from a clean slate
	// (either a fresh install, or a user who logged out fully).
	IsNewTokenAccordingToClientApp bool
}

func (c ConfigEncryptionInfo) String() string {
	return utils.Sprintf("Token hash %s (isNewTokenAccordingToClientApp: %v)", hash(c.Token), c.IsNewTokenAccordingToClientApp)
}

// StartLoadingUserConfigs starts the process of loading user configs. It
// returns immediately, providing a channel used to send the
// configEncryptionToken once it's available (to start the decryption process),
// and a channel that returns errors with the process as they occur. The
// configEncryptionToken channel should only ever have exactly one value sent
// over it, but the error channel might potentially have multiple errors to
// report. The error channel will close to signify the completion of loading
// user configs. This design allows the config decryption to occur in parallel
// with the other steps to spinning up a mandelbox, while also providing the
// synchronization necessary.
func (mandelbox *mandelboxData) StartLoadingUserConfigs() (chan<- ConfigEncryptionInfo, <-chan error) {
	// We buffer both channels to prevent blocking.
	tokenChan := make(chan ConfigEncryptionInfo, 1)
	errorChan := make(chan error, 10)

	// This inner function is where we perform all the real work of loading user configs.
	go mandelbox.loadUserConfigs(tokenChan, errorChan)

	return tokenChan, errorChan
}

// loadUserConfigs does the "real work" of LoadUserConfigs. For each
// deviation from the happy path in this function, we need to decide whether it
// is a warning or an error — warnings get logged here, and errors get sent
// back.
func (mandelbox *mandelboxData) loadUserConfigs(tokenChan <-chan ConfigEncryptionInfo, errorChan chan<- error) {
	// TODO: pass in mandelbox as an argument in this and all child functions
	defer close(errorChan)

	// If userID is not set, then we don't retrieve configs from s3
	if len(mandelbox.GetUserID()) == 0 {
		logger.Warningf("User ID is not set for mandelbox %s. Skipping config download.", mandelbox.GetID())
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
	predictedConfig, err := mandelbox.predictConfigToDownload(s3Client)
	if err != nil {
		errorChan <- utils.MakeError("Error retrieving existing configs for user %s from S3 for mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}
	if predictedConfig == nil {
		logger.Warningf("There are no existing configs in s3 for user %s for mandelbox %s", mandelbox.GetUserID(), mandelbox.GetID())
		return
	}

	// Now that we have a prediction for the config file we need, we want to
	// download it into a buffer for decrypting.
	// TODO: Don't block on this unless we know that the config is indeed the one
	// we need. This will be more important as configs grow in size.
	predictedConfigBuf, err := mandelbox.downloadUserConfig(s3Client, *predictedConfig.Key)
	if err != nil {
		errorChan <- utils.MakeError("Error downloading predicted config %s for user %s and mandelbox %s: %s", *predictedConfig.Key, mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}

	// What we do want to block on is receiving the configEncryptionToken.
	encryptionInfo, err := mandelbox.receiveAndVerifyEncryptionInfo(tokenChan)
	if err != nil {
		errorChan <- err
		return
	}

	// Now we compare the config that we speculatively downloaded to the one we
	// know we need based on the token that came back from the client-app.

	// TODO: More logic here

	// TODO: no logic about mandelbox.SetConfigBuffer(configBuffer)
}

// predictConfigToDownload guesses which config is the most likely to be the
// one we need to download, given the user's config encryption token.
func (mandelbox *mandelboxData) predictConfigToDownload(s3Client *s3.Client) (*s3types.Object, error) {
	// We use the simple guess that the most recently-modified config is the one
	// that we need.
	return configutils.GetMostRecentMatchingKey(s3Client, UserConfigS3Bucket, mandelbox.getS3ConfigKeyPrefix(), EncryptedArchiveFilename)
}

// getS3ConfigKeyPrefix returns the name of the S3 key to the encrypted user
// config file, up to the app name. This does not include the suffix
// (`EncryptedArchiveFileName`) or the token hash that comes before it.
func (mandelbox *mandelboxData) getS3ConfigKeyPrefix() string {
	return path.Join(string(mandelbox.GetUserID()), metadata.GetAppEnvironmentLowercase(), string(mandelbox.GetAppName()))
}

// getS3ConfigPath returns the full key name of the S3 key to encrypted user
// config file. Note that this includes the hash of the user config.
func (mandelbox *mandelboxData) getS3ConfigKey(tokenHash string) string {
	return path.Join(mandelbox.getS3ConfigKeyPrefix(), "/", tokenHash, "/", EncryptedArchiveFilename)
}

// downloadUserConfig downloads a given user config from S3 into an in-memory buffer.
func (mandelbox *mandelboxData) downloadUserConfig(s3Client *s3.Client, key string) ([]byte, error) {
	// Fetch the HeadObject first to see how much memory we need to allocate
	headObject, err := configutils.GetHeadObject(s3Client, UserConfigS3Bucket, key)
	if err != nil {
		return nil, utils.MakeError("Could not get head object for key %s for mandelbox %s: %s", key, mandelbox.GetID(), err)
	}

	// Log config version
	logger.Infof("Attempting to download user config key %s (version %s) for mandelbox %s", *headObject.VersionId, mandelbox.GetID())

	// Download file into a pre-allocated in-memory buffer
	// This should be okay as we don't expect configs to be very large
	buf := manager.NewWriteAtBuffer(make([]byte, headObject.ContentLength))
	numBytes, err := configutils.DownloadObjectToBuffer(s3Client, UserConfigS3Bucket, key, buf)
	if err != nil {
		return nil, utils.MakeError("Could not download object for key %s (version %s) for mandelbox %s: %s", key, *headObject.VersionId, mandelbox.GetID(), err)
	}

	logger.Infof("Downloaded %v bytes for user config key %s (version %s) for mandelbox %s", numBytes, key, *headObject.VersionId, mandelbox.GetID())

	return buf.Bytes(), nil
}

// receiveEncryptionToken blocks until it receives the encryption token info
// from `tokenChan` (or at least until the channel is closed first, in which
// case it returns an error). It also performs some basic sanity checks on the
// token.
func (mandelbox *mandelboxData) receiveAndVerifyEncryptionInfo(tokenChan <-chan ConfigEncryptionInfo) (*ConfigEncryptionInfo, error) {
	encryptionInfo, gotEncryptionInfo := <-tokenChan
	if !gotEncryptionInfo {
		return nil, utils.MakeError("Got no config encryption token for user %s for mandelbox %s, likely because the JSON Transport request never completed.", mandelbox.GetUserID(), mandelbox.GetID())
	}

	if encryptionInfo.Token == "" {
		return nil, utils.MakeError("Got an empty config encryption token from the client-app for user %s for mandelbox %s", mandelbox.GetUserID(), mandelbox.GetID())
	}

	if len(encryptionInfo.Token) < 20 {
		return nil, utils.MakeError("Got a too-short config encryption (length %v) token from the client-app for user %s for mandelbox %s", len(encryptionInfo.Token), mandelbox.GetUserID(), mandelbox.GetID())
	}

	// The encryption token _looks_ reasonable. We set it for the mandelbox and log some info.
	mandelbox.SetConfigEncryptionToken(encryptionInfo.Token)

	logger.Infof("Got config encryption info for user %s and mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), encryptionInfo)

	return &encryptionInfo, nil
}

func (mandelbox *mandelboxData) evaluatePredictedConfigCorrectness(predictedConfig *s3types.Object, encryptionInfo ConfigEncryptionInfo) {

}

// Helpers

const (
	aes256KeyLength = 32
	aes256IVLength  = 16
)

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
	logger.Infof("Using (hashed) decryption token %s for mandelbox %s", hash(mandelbox.GetConfigEncryptionToken()), mandelbox.GetID())

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
	logger.Infof("Using (hashed) encryption token %s for mandelbox %s", hashConfigEncryptionToken(string(configToken)), mandelboxID)
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

	uploadResult, err := configutils.UploadFileToBucket(s3Client, UserConfigS3Bucket, mandelbox.getS3ConfigKey(), encryptedConfigBuffer)
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

// hash returns a human-readable hash of the given token.
func hash(token types.ConfigEncryptionToken) string {
	hash := md5.Sum([]byte(token))
	return base32.StdEncoding.EncodeToString(hash[:])
}
