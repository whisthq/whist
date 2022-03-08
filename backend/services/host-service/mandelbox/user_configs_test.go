package mandelbox

import (
	"bytes"
	"context"
	"os"
	"os/exec"
	"path"
	"strings"
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
			// Delete the user config directory so it can be recreated. Note that we
			// reuse `unpackedConfigPath` between the uploader and downloader
			// mandelboxes, so we rely on the userIDs and mandelboxIDs being the
			// same.
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
			contents, err := os.ReadDir(unpackedConfigPath)
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
}

// TestUserConfigChecksum tests the checksum verification functonality of
// the upload and download user config code.
func TestUserConfigChecksum(t *testing.T) {
	testFile1 := []byte("this is a test file")
	testFile1Hash := configutils.GetMD5Hash(testFile1)

	// Test upload
	s3Client, err := configutils.NewS3Client("us-east-1")
	if err != nil {
		t.Fatalf("failed to create s3 client: %v", err)
	}

	_, err = configutils.UploadFileToBucket(s3Client, UserConfigS3Bucket, "user_config_test_user/checksum_test", testFile1)
	if err != nil {
		t.Fatalf("failed to upload test file: %v", err)
	}

	// Check that checksum is present and correct
	head, err := configutils.GetHeadObject(s3Client, UserConfigS3Bucket, "user_config_test_user/checksum_test")
	if err != nil {
		t.Fatalf("failed to get head object: %v", err)
	}

	metadata := head.Metadata
	downloadHash, ok := metadata["md5"]
	if !ok {
		t.Fatalf("md5 metadata not present on head object")
	}

	if downloadHash != testFile1Hash {
		t.Fatalf("md5 metadata on head object does not match file hash")
	}

	// Test download happy path
	mandelbox := mandelboxData{ID: types.MandelboxID(utils.PlaceholderTestUUID())}
	data, err := mandelbox.downloadUserConfig(s3Client, "user_config_test_user/checksum_test", head)
	if err != nil {
		t.Errorf("failed to download file: %v", err)
	}

	if !bytes.Equal(data, testFile1) {
		t.Errorf("downloaded file does not match original file")
	}

	// Test download with bad checksum
	head.Metadata["md5"] = "bad_hash"
	_, err = mandelbox.downloadUserConfig(s3Client, "user_config_test_user/checksum_test", head)
	if err == nil || !strings.HasPrefix(err.Error(), "Could not download object (due to MD5 mismatch)") {
		t.Errorf("download with bad checksum did not fail")
	}
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return os.RemoveAll(utils.WhistDir)
}
