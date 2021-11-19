package mandelbox

import (
	"bytes"
	"context"
	"errors"
	"os"
	"os/exec"
	"path"
	"testing"

	"github.com/fractal/fractal/host-service/mandelbox/configutils"
	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/utils"
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

	unpackedConfigPath := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())
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

// TestUserInitialBrowserWrite checks if the browser data is properly created by
// calling the write function and comparing results with a manually generated cookie file
func TestUserInitialBrowserWrite(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	testMandelboxData := mandelboxData{
		ctx:                   ctx,
		cancel:                cancel,
		ID:                    mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		appName:               "testApp",
		userID:                "user_config_test_user",
		configEncryptionToken: "testEncryptionToken",
	}

	testCookie1 := "{'creation_utc': 13280861983875934, 'host_key': 'test_host_key_1.com'}"
	testCookie2 := "{'creation_utc': 4228086198342934, 'host_key': 'test_host_key_2.com'}"
	cookieJSON := "[" + testCookie1 + "," + testCookie2 + "]"

	bookmarksJSON := "{ 'test_bookmark_content': '1'}"

	if err := testMandelboxData.WriteUserInitialBrowserData(cookieJSON, bookmarksJSON); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// store the browser configs in a temporary file
	unpackedConfigDir := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())

	// Get browser data file path
	cookieFilePath := path.Join(unpackedConfigDir, utils.UserInitialCookiesFile)
	bookmarkFilePath := path.Join(unpackedConfigDir, utils.UserInitialBookmarksFile)

	// Stores the file path and content for each browser data type
	fileAndContents := [][]string{{cookieFilePath, cookieJSON}, {bookmarkFilePath, bookmarksJSON}}

	for _, fileAndContent := range fileAndContents {
		filePath := fileAndContent[0]
		testFileContent := fileAndContent[1]

		matchingFile, err := os.Open(filePath)
		if err != nil {
			t.Fatalf("error opening matching file %s: %v", filePath, err)
		}

		matchingFileBuf := bytes.NewBuffer(nil)
		_, err = matchingFileBuf.ReadFrom(matchingFile)
		if err != nil {
			t.Fatalf("error reading matching file %s: %v", filePath, err)
		}

		// Check contents match
		if string(testFileContent) != matchingFileBuf.String() {
			t.Errorf("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContent, matchingFileBuf.Bytes())
		}
	}

	os.RemoveAll(unpackedConfigDir)
}

// TestUserInitialBrowserWriteEmpty checks if passing empty browser data will result in no files generated
func TestUserInitialBrowserWriteEmpty(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	testMandelboxData := mandelboxData{
		ctx:                   ctx,
		cancel:                cancel,
		ID:                    mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		appName:               "testApp",
		userID:                "user_config_test_user_empty_browser_datas",
		configEncryptionToken: "testEncryptionToken",
	}

	// Empty browser data will not generate any files
	if err := testMandelboxData.WriteUserInitialBrowserData("", ""); err != nil {
		t.Fatalf("error writing empty user initial browser data: %v", err)
	}

	// store the browser configs in a temporary file
	unpackedConfigDir := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())

	if _, err := os.Stat(unpackedConfigDir); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data. Expected %v but got %v", os.ErrNotExist, err)
		os.RemoveAll(unpackedConfigDir)
	}
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return os.RemoveAll(utils.WhistDir)
}
