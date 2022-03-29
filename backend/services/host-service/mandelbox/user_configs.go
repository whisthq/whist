package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

// This file provides functions that manage user configs, including fetching, uploading, and encrypting them.

import (
	"context"
	"crypto/md5"
	"encoding/base32"
	"errors"
	"os"
	"os/exec"
	"path"
	"strings"
	"sync"

	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	s3types "github.com/aws/aws-sdk-go-v2/service/s3/types"
	"github.com/aws/smithy-go"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	types "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// Exported members

const (
	// UnpackedConfigsDirectoryName is the name of the directory in the mandelbox that stores
	// unpacked user configs.
	UnpackedConfigsDirectoryName = "unpacked_configs/"

	// EncryptedArchiveFilename is the name of the encrypted user config file.
	EncryptedArchiveFilename = "whist-app-config.tar.lz4.enc"
)

// ConfigEncryptionInfo defines the information we want from the client-app to
// successfully decrypt configs.
type ConfigEncryptionInfo struct {
	Token types.ConfigEncryptionToken
	// When IsNewTokenAccordingToClientApp is true, then the client app
	// effectively believes that the client-app is starting from a clean slate
	// (either a fresh install, or a user who logged out fully).
	IsNewTokenAccordingToClientApp bool
}

func (c ConfigEncryptionInfo) String() string {
	return utils.Sprintf("Token with length %v and hash %s (isNewTokenAccordingToClientApp: %v)", len(c.Token), hash(c.Token), c.IsNewTokenAccordingToClientApp)
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
// necessary synchronization.
func (mandelbox *mandelboxData) StartLoadingUserConfigs(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) (chan<- ConfigEncryptionInfo, <-chan error) {
	// We buffer both channels to prevent blocking.
	encryptionInfoChan := make(chan ConfigEncryptionInfo, 1)
	errorChan := make(chan error, 10)

	// This inner function is where we perform all the real work of loading user configs.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()
		mandelbox.loadUserConfigs(encryptionInfoChan, errorChan)
	}()

	return encryptionInfoChan, errorChan
}

// BackupUserConfigs compresses, encrypts, and then uploads user config files to S3.
func (mandelbox *mandelboxData) BackupUserConfigs() error {
	userID := mandelbox.GetUserID()
	configToken := mandelbox.GetConfigEncryptionToken()
	mandelboxID := mandelbox.GetID()

	if len(userID) == 0 {
		logger.Infof("Cannot save user configs for mandelbox %s since UserID is empty.", mandelboxID)
		return nil
	}

	if len(configToken) == 0 {
		return utils.MakeError("Cannot save user configs for user %s for mandelbox %s since ConfigEncryptionToken is empty", userID, mandelboxID)
	}

	configDir := mandelbox.GetUserConfigDir()
	unpackedConfigPath := path.Join(configDir, UnpackedConfigsDirectoryName)

	// Compress the user configs into a tar.lz4 file
	compressedConfig, err := configutils.CompressTarLz4(unpackedConfigPath)
	if err != nil {
		return utils.MakeError("Failed to compress configs for user %s for mandelbox %s: %v", userID, mandelboxID, err)
	}

	// Encrypt the compressed config
	logger.Infof("Using (hashed) encryption token %s for user %s for mandelbox %s", hash(configToken), userID, mandelboxID)
	encryptedConfig, err := configutils.EncryptAES256GCM(string(configToken), compressedConfig)
	if err != nil {
		return utils.MakeError("Failed to encrypt configs for user %s for mandelbox %s with hashed token %s: %s", userID, mandelboxID, hash(configToken), err)
	}

	// Upload encrypted config to S3
	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		return utils.MakeError("Error backing up user configs for user %s for mandelbox %s: error creating s3 client: %s", userID, mandelboxID, err)
	}

	uploadResult, err := configutils.UploadFileToBucket(s3Client, configutils.GetConfigBucket(), mandelbox.getS3ConfigKey(hash(configToken)), encryptedConfig)
	if err != nil {
		return utils.MakeError("Error uploading encrypted config for user %s for mandelbox %s to s3: %s", userID, mandelboxID, err)
	}

	logger.Infof("Successfully saved config version %s for user %s for mandelbox %s", *uploadResult.VersionID, userID, mandelboxID)
	return nil
}

// Helpers

// loadUserConfigs does the "real work" of LoadUserConfigs. For each
// deviation from the happy path in this function, we need to decide whether it
// is a warning or an error — warnings get logged here, and errors get sent
// back.
func (mandelbox *mandelboxData) loadUserConfigs(tokenChan <-chan ConfigEncryptionInfo, errorChan chan<- error) {
	// We don't want to close errorChan (which indicates to the consumer that
	// this function is "done") too early, but it's the last thing we want to do
	// from this function. No matter what, we want to verify the token/encryption
	// info that was passed in). Therefore, we block until the token verification
	// goroutine is done before closing errorChan, and we defer it right here.
	loadingWaitgroup := &sync.WaitGroup{}
	defer func() {
		logger.Infof("loadUserConfigs(): Waiting for loadingWaitgroup to be done...")
		loadingWaitgroup.Wait()
		close(errorChan)
	}()

	// If the process fails anywhere, we want to at least set up the empty config
	// directories for the mandelbox. We do this by setting `loadFailed` to true
	// until all the steps are done, and then creating the empty config
	// directories on function completion if `loadFailed` is still set to true.
	var loadFailed bool = true
	defer func() {
		if loadFailed {
			err := mandelbox.setupUserConfigDirs()
			if err != nil {
				errorChan <- utils.MakeError("Failed to last-resort setup user config dirs for mandelbox %s: %s", mandelbox.GetID(), err)
				return
			} else {
				logger.Infof("Set up last-resort user config dirs for mandelbox %s because normal loading failed.", mandelbox.GetID())
			}
		}
	}()

	// We also want to receive and set the config encryption token, even if the
	// process fails before where we would normally block waiting on this info.
	// Therefore, we begin blocking for it immediately and simply pass the
	// message along (after some verification) for the body of loadUserConfigs()
	// to wait on it later.
	// Note that `verifiedTokenChan` needs to be buffered in case the outer function returns before reading from it.
	verifiedTokenChan := make(chan ConfigEncryptionInfo, 1)
	loadingWaitgroup.Add(1)
	go func() {
		defer loadingWaitgroup.Done()
		defer close(verifiedTokenChan)
		encryptionInfo, err := mandelbox.receiveAndSanityCheckEncryptionToken(tokenChan)
		if err != nil {
			errorChan <- err
			// Here we only return from the inner function, but since we close
			// `verifiedTokenChan` the entire outer function will not block.
			return
		}
		verifiedTokenChan <- *encryptionInfo
	}()

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
	predictedConfigObj, err := mandelbox.predictConfigToDownload(s3Client)
	if err != nil {
		errorChan <- utils.MakeError("Error retrieving existing configs for user %s from S3 for mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}
	if predictedConfigObj == nil {
		logger.Warningf("There are no existing configs in s3 for user %s for mandelbox %s", mandelbox.GetUserID(), mandelbox.GetID())
		return
	}

	// Now that we have a prediction for the config file we need, we want to
	// download it into a buffer for decrypting.
	// TODO: Don't wait on this unless we know that the config is indeed the one
	// we need. This is not a big deal now, but will be more important as configs
	// grow in size, if we see users alternating between encryption tokens
	// frequently (which should never happen with the current setup).
	// (https://github.com/whisthq/whist/issues/5287)
	predictedConfigHeadObj, err := configutils.GetHeadObject(s3Client, configutils.GetConfigBucket(), *predictedConfigObj.Key)
	if err != nil {
		errorChan <- utils.MakeError("Could not get head object for predicted key %s for user %s for mandelbox %s: %s", *predictedConfigObj.Key, mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}
	predictedConfigBuf, err := mandelbox.downloadUserConfig(s3Client, *predictedConfigObj.Key, predictedConfigHeadObj)
	if err != nil {
		errorChan <- utils.MakeError("Error downloading predicted config %s for user %s and mandelbox %s: %s", *predictedConfigObj.Key, mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}

	// What we do want to block on is receiving the configEncryptionToken.
	encryptionInfo, ok := <-verifiedTokenChan
	if !ok {
		// Error already put in errorChan by the inner function above.
		return
	}

	// Now we compare the config that we speculatively downloaded to the one we
	// know we need based on the token that came back from the client-app.
	correctConfigKey, correctHeadObject, err := mandelbox.determineCorrectConfigKey(s3Client, *predictedConfigObj.Key, predictedConfigHeadObj, encryptionInfo)
	if err != nil {
		errorChan <- utils.MakeError("Could not determine correct config key for user %s for mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), err)
		return
	}

	// Check for the frontend's indication that this is a fresh config before
	// we do any more expensive operations.
	if encryptionInfo.IsNewTokenAccordingToClientApp {
		logger.Warningf("Client-app says the hashed config encryption token %s for user %s and mandelbox %s is new. Therefore, we will not try to decrypt any configs.", hash(encryptionInfo.Token), mandelbox.GetUserID(), mandelbox.GetID())
		return
	}

	// Do some basic sanity checks
	if correctConfigKey != mandelbox.getS3ConfigKey(hash(encryptionInfo.Token)) && correctConfigKey != path.Join(mandelbox.getS3ConfigKeyPrefix(), EncryptedArchiveFilename) {
		// If not one of those file paths, then we're about to try to decrypt the wrong file.
		errorChan <- utils.MakeError("Corrected config key %s for user %s for mandelbox %s does not match any expected format (prefix/filename or prefix/token/filename)!", correctConfigKey, mandelbox.GetUserID(), mandelbox.GetID())
		return
	}

	// If we need to, download the correct config archive from S3
	var correctConfigBuf []byte
	if correctConfigKey != *predictedConfigObj.Key {
		correctConfigBuf, err = mandelbox.downloadUserConfig(s3Client, correctConfigKey, correctHeadObject)
		if err != nil {
			errorChan <- utils.MakeError("Error downloading corrected config %s for user %s and mandelbox %s: %s", *predictedConfigObj.Key, mandelbox.GetUserID(), mandelbox.GetID(), err)
			return
		}
	} else {
		correctConfigBuf = predictedConfigBuf
	}

	// Decrypt the downloaded config, then unpack it
	decryptedConfig, err := mandelbox.decryptConfig(correctConfigKey, correctConfigBuf, encryptionInfo)
	if err != nil {
		errorChan <- err
		return
	}
	err = mandelbox.extractConfig(correctConfigKey, decryptedConfig)
	if err != nil {
		errorChan <- err
		return
	}

	loadFailed = false
}

// predictConfigToDownload guesses which config is the most likely to be the
// one we need to download, given the user's config encryption token.
func (mandelbox *mandelboxData) predictConfigToDownload(s3Client *s3.Client) (*s3types.Object, error) {
	// We use the simple guess that the most recently-modified config is the one
	// that we need.
	return configutils.GetMostRecentMatchingKey(s3Client, configutils.GetConfigBucket(), mandelbox.getS3ConfigKeyPrefix(), EncryptedArchiveFilename)
}

// getS3ConfigKeyPrefix returns the name of the S3 key to the encrypted user
// config file, up to the app name. This does not include the suffix
// (`EncryptedArchiveFileName`).
func (mandelbox *mandelboxData) getS3ConfigKeyPrefix(tokenHash string) string {
	return path.Join(tokenHash, string(mandelbox.GetUserID()), string(mandelbox.GetAppName()))
}

// getS3ConfigPath returns the full key name of the S3 key to encrypted user
// config file. Note that this includes the hash of the user config.
func (mandelbox *mandelboxData) getS3ConfigKey(tokenHash string) string {
	return path.Join(mandelbox.getS3ConfigKeyPrefix(tokenHash), "/", EncryptedArchiveFilename)
}

// downloadUserConfig downloads a given user config from S3 into an in-memory
// buffer. It takes in key and the HeadObjectOutput for the file to be
// downloaded (for size and version info) and returns the byte slice containing
// the object and any errors.
func (mandelbox *mandelboxData) downloadUserConfig(s3Client *s3.Client, key string, headObject *s3.HeadObjectOutput) ([]byte, error) {
	// Log config version
	logger.Infof("Attempting to download user config key %s (version %s) for mandelbox %s", key, *headObject.VersionId, mandelbox.GetID())

	// Get the md5 hash of the config file
	validateIntegrity := true
	configMetadata := headObject.Metadata
	expectedMD5, ok := configMetadata["md5"]
	if !ok {
		validateIntegrity = false
	}

	var downloadedFile []byte
	var downloadSuccessful bool
	var numBytesSuccessfullyDownloaded int64

	// Allow up to 3 retries on download if failure is because of checksum mismatch, this is
	// intended to solve issues with configs sometimes being malformed due to network issues
	for i := 0; i < 3; i++ {
		// Download file into a pre-allocated in-memory buffer
		// This should be okay as we don't expect configs to be very large
		buf := manager.NewWriteAtBuffer(make([]byte, headObject.ContentLength))
		numBytes, err := configutils.DownloadObjectToBuffer(s3Client, configutils.GetConfigBucket(), key, buf)
		if err != nil {
			return nil, utils.MakeError("Could not download object for key %s (version %s) for mandelbox %s: %s", key, *headObject.VersionId, mandelbox.GetID(), err)
		}

		// Check if the file was downloaded correctly
		downloadedFile = buf.Bytes()
		downloadHash := configutils.GetMD5Hash(downloadedFile)
		if validateIntegrity && downloadHash != expectedMD5 {
			logger.Warningf("MD5 hash mismatch for user config key %s (version %s) for mandelbox %s. Expected %s, got %s", key, *headObject.VersionId, mandelbox.GetID(), expectedMD5, downloadHash)
			continue
		}

		downloadSuccessful = true
		numBytesSuccessfullyDownloaded = numBytes
		break
	}

	if !downloadSuccessful {
		return nil, utils.MakeError("Could not download object (due to MD5 mismatch) for key %s (version %s) for mandelbox %s after 3 attempts", key, *headObject.VersionId, mandelbox.GetID())
	}

	logger.Infof("Downloaded %v bytes for user config key %s (version %s) for mandelbox %s", numBytesSuccessfullyDownloaded, key, *headObject.VersionId, mandelbox.GetID())

	return downloadedFile, nil
}

// receiveAndSanityCheckEncryptionToken blocks until it receives the encryption
// token info from `uncheckedTokenChan` (or at least until the channel is
// closed first, which it treats as an error). It also performs some basic
// sanity checks on the token (e.g to make sure it's not empty), and if they
// pass, sets the receiving mandelbox's config token to the provided value.
func (mandelbox *mandelboxData) receiveAndSanityCheckEncryptionToken(uncheckedTokenChan <-chan ConfigEncryptionInfo) (*ConfigEncryptionInfo, error) {
	encryptionInfo, gotEncryptionInfo := <-uncheckedTokenChan
	if !gotEncryptionInfo {
		return nil, utils.MakeError("Got no config encryption token for user %s for mandelbox %s, likely because the JSON Transport request never completed.", mandelbox.GetUserID(), mandelbox.GetID())
	}

	if encryptionInfo.Token == "" {
		return &encryptionInfo, utils.MakeError("Encryption token for user %s for mandelbox %s is empty!", mandelbox.GetUserID(), mandelbox.GetID())
	}

	if len(encryptionInfo.Token) < 15 {
		return &encryptionInfo, utils.MakeError("Encryption token for user %s for mandelbox %s is way too short (length %v)", mandelbox.GetUserID(), mandelbox.GetID(), len(encryptionInfo.Token))
	}

	// The encryption token _looks_ reasonable. We set it for the mandelbox and log some info.
	mandelbox.SetConfigEncryptionToken(encryptionInfo.Token)

	logger.Infof("Got config encryption info that passed sanity checks for user %s and mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), encryptionInfo)

	return &encryptionInfo, nil
}

// determineCorrectConfigKey confirms or corrects our predictions about which
// S3 object to download for configs, given the ConfigEncryptionInfo from the
// client-app.
func (mandelbox *mandelboxData) determineCorrectConfigKey(s3client *s3.Client, predictedKey string, predictedHeadObject *s3.HeadObjectOutput, encryptionInfo ConfigEncryptionInfo) (string, *s3.HeadObjectOutput, error) {
	tokenHash := hash(encryptionInfo.Token)

	// (happy path) If the predicted config's key contains the hash of the token,
	// then we know we're good to go.
	if strings.Contains(predictedKey, tokenHash) {
		logger.Infof("Predicted key %s to download configs for user %s for mandelbox %s is correct.", predictedKey, mandelbox.GetUserID(), mandelbox.GetID())
		return predictedKey, predictedHeadObject, nil
	}

	// If there is a config in s3 that contains the token hash, that's the one we desire.
	desiredKey := mandelbox.getS3ConfigKey(tokenHash)
	desiredHead, err := configutils.GetHeadObject(s3client, configutils.GetConfigBucket(), desiredKey)
	if err == nil {
		logger.Warningf("Predicted key %s to download configs for user %s for mandelbox %s were replaced with corrected key %s", predictedKey, mandelbox.GetUserID(), mandelbox.GetID(), desiredKey)
		return desiredKey, desiredHead, nil
	} else if err != nil {
		var apiErr smithy.APIError
		if !(errors.As(err, &apiErr) && apiErr.ErrorCode() == "NotFound") {
			// The desired file exists in S3, but we ran into some error getting the
			// HeadObject. Something went poorly.
			return predictedKey, predictedHeadObject, utils.MakeError("Couldn't get HeadObject for corrected config key %s (from predicted key %s) for user %s for mandelbox %s: %s", desiredKey, predictedKey, mandelbox.GetUserID(), mandelbox.GetID(), err)
		}
	}

	// There's no upstream key matching desiredKey (which is what we would have
	// expected if we'd have uploaded configs labeled with this token before).
	// Therefore, we just have to roll with our predictions.
	logger.Warningf("There is no key in S3 matching our desired config key %s for user %s for mandelbox %s. Continuing with predicted key %s for user configs.", desiredKey, mandelbox.GetUserID(), mandelbox.GetID(), predictedKey)
	return predictedKey, predictedHeadObject, nil
}

// decryptConfig decrypts the given buffer. This function assumes that
// `receiveAndVerifyEncryptionInfo` has already been called.
func (mandelbox *mandelboxData) decryptConfig(configKey string, encryptedBuf []byte, encryptionInfo ConfigEncryptionInfo) ([]byte, error) {
	if len(encryptionInfo.Token) == 0 {
		return nil, utils.MakeError("Cannot get user configs for user %s for Mandelbox %s since config encryption token is empty", mandelbox.GetUserID(), mandelbox.GetID())
	}

	if len(encryptedBuf) == 0 {
		return nil, utils.MakeError("Cannot decrypt length-0 buffer for user configs for user %s for Mandelbox %s since config buffer has length 0!", mandelbox.GetUserID(), mandelbox.GetID())
	}

	logger.Infof("Decrypting config %s for user %s for mandelbox %s with hashed token %s", configKey, mandelbox.GetUserID(), mandelbox.GetID(), hash(encryptionInfo.Token))

	decryptedBuf, err := configutils.DecryptAES256GCM(string(encryptionInfo.Token), encryptedBuf)
	if err != nil {
		return nil, utils.MakeError("Failed to decrypt config %s for user %s for mandelbox %s with hashed token %s: %s", configKey, mandelbox.GetUserID(), mandelbox.GetID(), hash(encryptionInfo.Token), err)
	}

	logger.Infof("Successfully decrypted %s for user %s for mandelbox %s with hashed token %s", configKey, mandelbox.GetUserID(), mandelbox.GetID(), hash(encryptionInfo.Token))

	return decryptedBuf, nil
}

// extractConfig extracts a decrypted config to the relevant directory.
func (mandelbox *mandelboxData) extractConfig(configKey string, decryptedConfig []byte) error {
	logger.Infof("Extracting config %s for user %s for mandelbox %s", configKey, mandelbox.GetUserID(), mandelbox.GetID())

	// Make directory for user configs
	configDir := mandelbox.GetUserConfigDir()
	unpackedConfigDir := path.Join(configDir, UnpackedConfigsDirectoryName)
	err := mandelbox.setupUserConfigDirs()
	if err != nil {
		return utils.MakeError("Failed to setup user config directories for user %s for mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), err)
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

	totalFileSize, err := configutils.ExtractTarLz4(decryptedConfig, unpackedConfigDir)
	if err != nil {
		logger.Errorf("Error extracting tar lz4 file for user %s for mandelbox %s: %s", mandelbox.GetUserID(), mandelbox.GetID(), err)
	}

	logger.Infof("Untarred config to: %s, total size was %d bytes", unpackedConfigDir, totalFileSize)

	return nil
}

// setupUserConfigDirs creates the user config directories on the host.
func (mandelbox *mandelboxData) setupUserConfigDirs() error {
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

// GetUserConfigDir returns the absolute path to the user config directory. It
// is exported for testing reasons and is not defined in the mandelbox
// interface.
func (mandelbox *mandelboxData) GetUserConfigDir() string {
	return path.Join(utils.WhistDir, mandelbox.GetID().String(), "userConfigs")
}

// hash returns a human-readable hash of the given token.
func hash(token types.ConfigEncryptionToken) string {
	hash := md5.Sum([]byte(token))
	return base32.StdEncoding.EncodeToString(hash[:])
}

// GetSavedExtensions returns the extensions that are saved in the config.
func (mandelbox *mandelboxData) GetSavedExtensions() []string {
	savedConfigsDir := path.Join(mandelbox.GetUserConfigDir(), UnpackedConfigsDirectoryName)
	return configutils.GetImportedExtensions(savedConfigsDir)
}

// WriteSavedExtensions updates the extensions that are saved in the config.
func (mandelbox *mandelboxData) WriteSavedExtensions(extensions []string) error {
	savedConfigsDir := path.Join(mandelbox.GetUserConfigDir(), UnpackedConfigsDirectoryName)
	return configutils.SaveImportedExtensions(savedConfigsDir, extensions)
}
