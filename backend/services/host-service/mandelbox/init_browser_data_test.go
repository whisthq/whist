package mandelbox

import (
	"bytes"
	"os"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// TestUserInitialBrowserWrite checks if the browser data is properly created by
// calling the write function and comparing results with a manually generated cookie file
func TestUserInitialBrowserWrite(t *testing.T) {
	// Define browser data
	testCookie1 := "{'creation_utc': 13280861983875934, 'host_key': 'test_host_key_1.com'}"
	testCookie2 := "{'creation_utc': 4228086198342934, 'host_key': 'test_host_key_2.com'}"

	// We will simulate a user with cookies but no bookmarks
	cookiesJSON := "[" + testCookie1 + "," + testCookie2 + "]"
	bookmarksJSON := ""
	extensions := "not_real_extension_id,not_real_second_extension_id"

	// Create browser data
	userInitialBrowserData := BrowserData{
		CookiesJSON:   types.Cookies(cookiesJSON),
		BookmarksJSON: types.Bookmarks(bookmarksJSON),
		Extensions:    types.Extensions(extensions),
	}

	// Explicitly set the result to what we expect
	testFileContent := utils.Sprintf(`{"cookiesJSON":"%s","extensions":"%s"}`, cookiesJSON, extensions)

	testMbox, _, _ := createTestMandelbox()
	defer os.RemoveAll(testMbox.GetUserConfigDir())

	if err := testMbox.WriteUserInitialBrowserData(userInitialBrowserData); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// Get browser data file path
	browserDataFile := path.Join(testMbox.GetUserConfigDir(), UnpackedConfigsDirectoryName, UserInitialBrowserFile)

	matchingFile, err := os.Open(browserDataFile)
	if err != nil {
		t.Fatalf("error opening matching file %s: %v", browserDataFile, err)
	}

	var matchingFileBuf bytes.Buffer
	_, err = matchingFileBuf.ReadFrom(matchingFile)
	if err != nil {
		t.Fatalf("error reading matching file %s: %v", browserDataFile, err)
	}

	// Check contents match
	if string(testFileContent) != matchingFileBuf.String() {
		t.Fatalf("file contents don't match for file %s: '%s' vs '%s'", browserDataFile, testFileContent, matchingFileBuf.Bytes())
	}
}

// TestUserInitialBrowserWriteEmpty checks if passing empty browser data will result in an empty json file
func TestUserInitialBrowserWriteEmpty(t *testing.T) {
	testMbox, _, _ := createTestMandelbox()
	defer os.RemoveAll(testMbox.GetUserConfigDir())

	// Empty browser data will generate an empty json file
	if err := testMbox.WriteUserInitialBrowserData(BrowserData{}); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// Get browser data file path
	browserDataFile := path.Join(testMbox.GetUserConfigDir(), UnpackedConfigsDirectoryName, UserInitialBrowserFile)

	matchingFile, err := os.Open(browserDataFile)
	if err != nil {
		t.Fatalf("error opening matching file %s: %v", browserDataFile, err)
	}

	var matchingFileBuf bytes.Buffer
	_, err = matchingFileBuf.ReadFrom(matchingFile)
	if err != nil {
		t.Fatalf("error reading matching file %s: %v", browserDataFile, err)
	}

	// Check contents match
	if matchingFileBuf.String() != "{}" {
		t.Fatalf("file contents don't match for file %s: '{}' vs '%s'", browserDataFile, matchingFileBuf.Bytes())
	}
}
