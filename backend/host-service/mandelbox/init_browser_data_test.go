package mandelbox

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/core-go/types"
)

// TestUserInitialBrowserWrite checks if the browser data is properly created by
// calling the write function and comparing results with a manually generated cookie file
func TestUserInitialBrowserWrite(t *testing.T) {
	destDir, err := ioutil.TempDir("", "testInitBrowser")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(destDir)

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


	testFileContent := fmt.Sprintf(`{"cookiesJSON": "%s", "extensions": "%s"}`, cookiesJSON, extensions)

	if err := WriteUserInitialBrowserData(userInitialBrowserData, destDir); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// Get browser data file path
	browserDataFile := path.Join(destDir, UserInitialBrowserFile)

	matchingFile, err := os.Open(browserDataFile)
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

// TestUserInitialBrowserWriteEmpty checks if passing empty browser data will result in no files generated
func TestUserInitialBrowserWriteEmpty(t *testing.T) {
	destDir, err := ioutil.TempDir("", "testInitBrowser")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(destDir)

	// Empty browser data will not generate any files
	if err := WriteUserInitialBrowserData(BrowserData{}, destDir); err != nil {
		t.Fatalf("error writing empty user initial browser data: %v", err)
	}

	// Check if the files do not exists
	browserDataFile := path.Join(destDir, UserInitialBrowserFile)
	if _, err := os.Stat(browserDataFile); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data file. Expected %v but got %v", os.ErrNotExist, err)
	}
}
