package mandelbox

import (
	"context"
	"os/exec"
	"path"
	"sync"
	"testing"

	"github.com/spf13/afero"
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
	oldFs := utils.Fs
	utils.SetFs(afero.NewMemMapFs())

	err := uploaderMandelboxData.setupUserConfigDirs()
	if err != nil {
		t.Fatalf("failed to create user config directories: %v", err)
	}

	mandelboxDir := path.Join(utils.WhistDir, uploaderMandelboxData.ID.String())
	sourceDir := path.Join(mandelboxDir, "testBase")
	if err := fs.MkdirAll(sourceDir, 0777); err != nil {
		t.Fatalf("failed to create source directory: %v", err)
	}

	err = configutils.SetupTestDir(sourceDir)
	if err != nil {
		t.Fatalf("failed to set up test directories: %v", err)
	}

	unpackedConfigPath := path.Join(uploaderMandelboxData.GetUserConfigDir(), UnpackedConfigsDirectoryName)
	if err := fs.MkdirAll(unpackedConfigPath, 0777); err != nil {
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
			// Delete the user config directory so it can be recreated. Note that we
			// reuse `unpackedConfigPath` between the uploader and downloader
			// mandelboxes, so we rely on the userIDs and mandelboxIDs being the
			// same.
			fs.RemoveAll(unpackedConfigPath)

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

			// Verify errors (or their absence) and config directory file paths
			// depending on provided params
			errCount := 0
			for err := range errChan {
				t.Log(err)
				errCount++
			}

			if !isNewToken && useOriginalToken {
				// We expect no errors, and for configs to be loaded fully.
				if errCount != 0 {
					t.Fatalf("Got errors with happy path configuration!")
				}

				destinationPath := path.Join(unpackedConfigPath, "testBase")
				err = configutils.ValidateDirectoryContents(sourceDir, destinationPath)
				if err != nil {
					t.Fatalf("error validating directory contents: %v", err)
				}
				return
			}

			if isNewToken {
				// We expect no errors if we have a new token
				if errCount != 0 {
					t.Fatalf("Got errors with new token configuration!")
				}
			} else if !useOriginalToken && errCount == 0 {
				t.Fatalf("expected errors since invalid (not new) token supplied!")
			}

			// In all but the happy-path case, we expect to have set up the
			// "last-resort" empty config dirs.
			contents, err := afero.ReadDir(fs, unpackedConfigPath)
			if err != nil {
				t.Fatalf("Last-resort user config directories not created properly!")
			}
			if len(contents) > 0 {
				t.Fatalf("Last-resort unpacked user config directory not empty as it should be!")
			}
		}
	}

	t.Run("valid token", downloadTestNotNewToken(true, false))
	t.Run("invalid token", downloadTestNotNewToken(false, false))

	// With a new token, anything is valid. But we want to make sure
	t.Run("new repeated token", downloadTestNotNewToken(true, true))
	t.Run("new non-repeated token", downloadTestNotNewToken(false, true))

	utils.SetFs(oldFs)
}
