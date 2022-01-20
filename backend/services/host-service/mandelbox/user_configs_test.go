package mandelbox

import (
	"context"
	"os"
	"os/exec"
	"path"
	"sync"
	"testing"

	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// TestUserConfigIntegration is an integration test for the
// end-to-end user config pipeline. First we create a test
// user config directory structure with some files. Then
// we compress, encrypt, and upload to S3. Then we download,
// decrypt, and uncompress the file and assert that the final
// config is the same as the original.
func TestUserConfigIntegration(t *testing.T) {
	uCtx, uCancel := context.WithCancel(context.Background())
	testToken := types.ConfigEncryptionToken("testEncryptionToken")
	testInvalidToken := types.ConfigEncryptionToken("testInvalidEncryptionToken")
	uploaderMandelboxData := mandelboxData{
		ctx:                   uCtx,
		cancel:                uCancel,
		ID:                    types.MandelboxID(utils.PlaceholderTestUUID()),
		appName:               "testApp",
		userID:                "user_config_test_user",
		configEncryptionToken: testToken,
	}

	// Start with a clean slate
	os.RemoveAll(utils.WhistDir)

	err := uploaderMandelboxData.setupUserConfigDirs()
	if err != nil {
		t.Fatalf("failed to create user config directories: %v", err)
	}

	mandelboxDir := path.Join(utils.WhistDir, uploaderMandelboxData.ID.String())
	sourceDir := path.Join(mandelboxDir, "testBase")
	if err := os.MkdirAll(sourceDir, 0777); err != nil {
		t.Fatalf("failed to create source directory: %v", err)
	}

	err = configutils.SetupTestDir(sourceDir)
	if err != nil {
		t.Fatalf("failed to set up test directories: %v", err)
	}
	defer cleanupTestDirs()

	unpackedConfigPath := path.Join(uploaderMandelboxData.GetUserConfigDir(), UnpackedConfigsDirectoryName)
	if err := os.MkdirAll(unpackedConfigPath, 0777); err != nil {
		t.Fatalf("failed to create config dir %s: %v", unpackedConfigPath, err)
	}

	// Copy test directory to unpacked config path
	copyCommand := exec.Command("cp", "-R", sourceDir, unpackedConfigPath)
	output, err := copyCommand.CombinedOutput()
	if err != nil {
		t.Fatalf("error copying test directories: %v, output: %s", err, output)
	}

	if err := uploaderMandelboxData.BackupUserConfigs(); err != nil {
		t.Fatalf("error backing up configs: %v", err)
	}

	downloadTestNotNewToken := func(useOriginalToken bool, isNewToken bool) func(*testing.T) {
		return func(t *testing.T) {
			// Delete the user config directory so it can be recreated
			os.RemoveAll(unpackedConfigPath)

			dCtx, dCancel := context.WithCancel(context.Background())
			downloaderMandelboxData := mandelboxData{
				ctx:     dCtx,
				cancel:  dCancel,
				ID:      types.MandelboxID(utils.PlaceholderTestUUID()),
				appName: "testApp",
				userID:  "user_config_test_user",
			}

			ctx, cancel := context.WithCancel(context.Background())
			goroutineTracker := sync.WaitGroup{}
			infoChan, errChan := downloaderMandelboxData.StartLoadingUserConfigs(ctx, cancel, &goroutineTracker)
			defer close(infoChan)

			tokenToUse := func() types.ConfigEncryptionToken {
				if useOriginalToken {
					return testToken
				} else {
					return testInvalidToken
				}
			}()

			infoChan <- ConfigEncryptionInfo{
				Token:                          tokenToUse,
				IsNewTokenAccordingToClientApp: isNewToken,
			}

			// Verify errors (or their absence) and config directory file paths depending on provided params
			errCount := 0
			for err := range errChan {
				t.Log(err)
				errCount++
			}

			if isNewToken {
				// We expect no errors if we have a new token
				if errCount != 0 {
					t.Fatalf("Got errors with new token configuration!")
				}
			} else if !useOriginalToken && errCount == 0 {
				t.Fatalf("expected errors since invalid (not new) token supplied!")
			}

			// Verify that all files in original directory are still there and correct
			destinationPath := path.Join(unpackedConfigPath, "testBase")
			err = configutils.ValidateDirectoryContents(sourceDir, destinationPath)
			if err != nil {
				t.Fatalf("error validating directory contents: %v", err)
			}
		}
	}

	t.Run("valid token", downloadTestNotNewToken(true, false))
	t.Run("invalid token", downloadTestNotNewToken(false, false))

	// With a new token, anything is valid. But we want to make sure
	t.Run("new repeated token", downloadTestNotNewToken(true, true))
	t.Run("new non-repeated token", downloadTestNotNewToken(false, true))
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return os.RemoveAll(utils.WhistDir)
}
