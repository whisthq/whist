package mandelbox

import (
	"context"
	"os"
	"os/exec"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// TestUserConfigIntegration is an integration test for the
// end-to-end user config pipeline. First we create a test
// user config directory structure with some files. Then
// we compress, encrypt, and upload to S3. Then we download,
// decrypt, and uncompress the file and assert that the final
// config is the same as the original.
func TestUserConfigIntegration(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	testMandelboxData := mandelboxData{
		ctx:                   ctx,
		cancel:                cancel,
		ID:                    mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		appName:               "testApp",
		userID:                "user_config_test_user",
		configEncryptionToken: "testEncryptionToken",
	}

	// Start with a clean slate
	os.RemoveAll(utils.WhistDir)

	err := testMandelboxData.SetupUserConfigDirs()
	if err != nil {
		t.Fatalf("failed to create user config directories: %v", err)
	}

	mandelboxDir := path.Join(utils.WhistDir, testMandelboxData.ID.String())
	sourceDir := path.Join(mandelboxDir, "testBase")
	if err := os.MkdirAll(sourceDir, 0777); err != nil {
		t.Fatalf("failed to create source directory: %v", err)
	}

	err = configutils.SetupTestDir(sourceDir)
	if err != nil {
		t.Fatalf("failed to set up test directories: %v", err)
	}
	defer cleanupTestDirs()

	unpackedConfigPath := path.Join(testMandelboxData.GetUserConfigDir(), UnpackedConfigsDirectoryName)
	if err := os.MkdirAll(unpackedConfigPath, 0777); err != nil {
		t.Fatalf("failed to create config dir %s: %v", unpackedConfigPath, err)
	}

	// Copy test directory to unpacked config path
	copyCommand := exec.Command("cp", "-R", sourceDir, unpackedConfigPath)
	output, err := copyCommand.CombinedOutput()
	if err != nil {
		t.Fatalf("error copying test directories: %v, output: %s", err, output)
	}

	if err := testMandelboxData.BackupUserConfigs(); err != nil {
		t.Fatalf("error backing up configs: %v", err)
	}

	t.Run("valid token", func(t *testing.T) {
		// Delete the user config directory so it can be recreated
		os.RemoveAll(unpackedConfigPath)

		if err := testMandelboxData.DownloadUserConfigs(); err != nil {
			t.Fatalf("error populating configs: %v", err)
		}

		if err := testMandelboxData.DecryptUserConfigs(); err != nil {
			t.Fatalf("error decrypting configs: %v", err)
		}

		// Verify that all files in original directory are still there and correct
		destinationPath := path.Join(unpackedConfigPath, "testBase")
		err = configutils.ValidateDirectoryContents(sourceDir, destinationPath)
		if err != nil {
			t.Fatalf("error validating directory contents: %v", err)
		}
	})

	t.Run("invalid token", func(t *testing.T) {
		// Delete the user config directory so it can be recreated
		os.RemoveAll(unpackedConfigPath)

		// Change token to be invalid
		testMandelboxData.configEncryptionToken = "invalidToken"

		if err := testMandelboxData.DownloadUserConfigs(); err != nil {
			t.Fatalf("error populating configs: %v", err)
		}

		if err := testMandelboxData.DecryptUserConfigs(); err == nil {
			t.Fatalf("expected error decrypting configs but got nil")
		}
	})
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return os.RemoveAll(utils.WhistDir)
}
