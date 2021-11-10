package mandelbox

import (
	"bytes"
	"context"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strings"
	"testing"

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
	os.RemoveAll(utils.FractalDir)

	err := setupTestDirs(&testMandelboxData)
	if err != nil {
		t.Fatalf("failed to set up test directories: %v", err)
	}
	defer cleanupTestDirs(&testMandelboxData)

	unpackedConfigPath := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())
	if err := os.MkdirAll(unpackedConfigPath, 0777); err != nil {
		t.Fatalf("failed to create config dir %s: %v", unpackedConfigPath, err)
	}

	testBase := path.Join(utils.FractalDir, testMandelboxData.ID.String(), "testBase")

	// Copy test directory to unpacked config path
	copyCommand := exec.Command("cp", "-R", testBase, unpackedConfigPath)
	output, err := copyCommand.CombinedOutput()
	if err != nil {
		t.Fatalf("error copying test directories: %v, output: %s", err, output)
	}

	testMandelboxData.rwlock.Lock()
	if err := testMandelboxData.BackupUserConfigs(); err != nil {
		t.Fatalf("error backing up configs: %v", err)
	}
	testMandelboxData.rwlock.Unlock()

	// Delete the user config directory so it can be recreated
	os.RemoveAll(unpackedConfigPath)

	if err := testMandelboxData.DownloadUserConfigs(); err != nil {
		t.Fatalf("error populating configs: %v", err)
	}

	if err := testMandelboxData.DecryptUserConfigs(); err != nil {
		t.Fatalf("error decrypting configs: %v", err)
	}

	// Verify that all files in original directory are still there and correct
	err = filepath.Walk(testBase, func(filePath string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		relativePath := strings.ReplaceAll(filePath, testBase, "")
		unpackedPath := path.Join(unpackedConfigPath, "testBase", relativePath)
		matchingFile, err := os.Open(unpackedPath)
		if err != nil {
			t.Fatalf("error opening matching file %s: %v", unpackedPath, err)
		}
		defer matchingFile.Close()

		matchingFileInfo, err := matchingFile.Stat()
		if err != nil {
			t.Fatalf("error reading stat for file: %s: %v", unpackedPath, err)
		}

		// If one is a directory, both should be
		if info.IsDir() {
			if !matchingFileInfo.IsDir() {
				t.Errorf("expected %s to be a directory", unpackedPath)
			}
		} else {
			testFileContents, err := ioutil.ReadFile(filePath)
			if err != nil {
				t.Fatalf("error reading test file %s: %v", filePath, err)
			}

			matchingFileBuf := bytes.NewBuffer(nil)
			_, err = matchingFileBuf.ReadFrom(matchingFile)
			if err != nil {
				t.Fatalf("error reading matching file %s: %v", unpackedPath, err)
			}

			// Check contents match
			if string(testFileContents) != string(matchingFileBuf.Bytes()) {
				t.Errorf("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContents, matchingFileBuf.Bytes())
			}
		}

		return nil
	})
	if err != nil {
		t.Fatalf("error walking filetree: %v", err)
	}
}

// setupTestDirs creates a sample user config with some nested directories
// and files inside.
func setupTestDirs(mandelbox *mandelboxData) error {
	configDir := path.Join(utils.FractalDir, mandelbox.ID.String())
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return utils.MakeError("failed to create config dir %s: %v", configDir, err)
	}

	testDir := path.Join(configDir, "testBase")
	if err := os.Mkdir(testDir, 0777); err != nil {
		return utils.MakeError("failed to create test base dir %s: %v", testDir, err)
	}

	// Write some directories with text files
	for i := 0; i < 300; i++ {
		tempDir := path.Join(testDir, utils.Sprintf("dir%d", i))
		if err := os.Mkdir(tempDir, 0777); err != nil {
			return utils.MakeError("failed to create temp dir %s: %v", tempDir, err)
		}

		for j := 0; j < 3; j++ {
			filePath := path.Join(tempDir, utils.Sprintf("file-%d.txt", j))
			fileContents := utils.Sprintf("This is file %d in directory %d.", j, i)
			err := os.WriteFile(filePath, []byte(fileContents), 0777)
			if err != nil {
				return utils.MakeError("failed to write to file %s: %v", filePath, err)
			}
		}
	}

	// Write a nested directory with files inside
	nestedDir := path.Join(testDir, "dir10", "nested")
	if err := os.Mkdir(nestedDir, 0777); err != nil {
		return utils.MakeError("failed to create nested dir %s: %v", nestedDir, err)
	}
	nestedFile := path.Join(nestedDir, "nested-file.txt")
	fileContents := utils.Sprintf("This is a nested file.")
	err := os.WriteFile(nestedFile, []byte(fileContents), 0777)
	if err != nil {
		return utils.MakeError("failed to write to file %s: %v", nestedFile, err)
	}

	// Write some files not in a directory
	for i := 0; i < 5; i++ {
		filePath := path.Join(testDir, utils.Sprintf("file-%d.txt", i))
		fileContents := utils.Sprintf("This is file %d in directory %s.", i, "none")
		err := os.WriteFile(filePath, []byte(fileContents), 0777)
		if err != nil {
			return utils.MakeError("failed to write to file %s: %v", filePath, err)
		}
	}

	return nil
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs(c *mandelboxData) error {
	return os.RemoveAll(utils.FractalDir)
}

// TestUserInitialBrowserWrite checks if the browser data is properly created by
// calling the write function and comparing results with a manually generated cookie file
func TestUserInitialBrowserWrite(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	testMandelboxData := mandelboxData{
		ctx:                   ctx,
		cancel:                cancel,
		ID:                    "userConfigTest",
		appName:               "testApp",
		userID:                "user_config_test_user",
		configEncryptionToken: "testEncryptionToken",
	}
	
	whistCookie := "{'creation_utc': 13280861983875934, 'host_key': 'whist.com'}"
	fractalCookie := "{'creation_utc': 4228086198342934, 'host_key': 'fractal.co'}"
	cookieJSON := "[" + whistCookie + "," + fractalCookie + "]"

	if err := testMandelboxData.WriteUserInitialBrowserData(cookieJSON); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// store the cookies in a temporary file
	filePath := path.Join(utils.UserInitialCookiesDir, "tempt-cookies")

	fileContents := whistCookie+"\n"+fractalCookie+"\n"

	err := os.WriteFile(filePath, []byte(fileContents), 0777)
	
	testFileContents, err := ioutil.ReadFile(filePath)
	
	if err != nil {
		t.Fatalf("error reading test file %s: %v", filePath, err)
	}

	matchingFileBuf := bytes.NewBuffer(nil)

	unpackedConfigDir := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())

	cookieFilePath := path.Join(unpackedConfigDir, utils.UserInitialCookiesFile)

	_, err = matchingFileBuf.ReadFrom(cookieFilePath)
	if err != nil {
		t.Fatalf("error reading matching file %s: %v", unpackedPath, err)
	}

	// Check contents match
	if string(testFileContents) != string(matchingFileBuf.Bytes()) {
		t.Errorf("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContents, matchingFileBuf.Bytes())
	}

	os.RemoveAll(utils.UserInitialCookiesDir)
}
