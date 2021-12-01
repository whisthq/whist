package initialbrowserdata

import (
	"bytes"
	"context"
	"errors"
	"os"
	"path"
	"testing"

	mandelboxData "github.com/fractal/fractal/host-service/mandelbox"
	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/utils"
)

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

	// Create browser data
	userInitialBrowserData := BrowserData{
		CookieJSON: cookieJSON
		BookmarkJSON: bookmarksJSON
	}
	if err := testMandelboxData.WriteUserInitialBrowserData(userInitialBrowserData); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// store the browser configs in a temporary file
	unpackedConfigDir := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())

	// Get browser data file path
	cookieFilePath := path.Join(unpackedConfigDir, UserInitialCookiesFile)
	bookmarkFilePath := path.Join(unpackedConfigDir, UserInitialBookmarksFile)

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
	if err := testMandelboxData.WriteUserInitialBrowserData(BrowserData{}); err != nil {
		t.Fatalf("error writing empty user initial browser data: %v", err)
	}

	// store the browser configs in a temporary file
	unpackedConfigDir := path.Join(testMandelboxData.getUserConfigDir(), testMandelboxData.getUnpackedConfigsDirectoryName())

	if _, err := os.Stat(unpackedConfigDir); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data. Expected %v but got %v", os.ErrNotExist, err)
		os.RemoveAll(unpackedConfigDir)
	}
}