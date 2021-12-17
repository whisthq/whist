package mandelbox

import (
	"bytes"
	"errors"
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

	if err := WriteUserInitialBrowserData(userInitialBrowserData, destDir); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// Get browser data file path
	cookieFilePath := path.Join(destDir, UserInitialCookiesFile)
	extensionFilePath := path.Join(destDir, UserInitialExtensionsFile)

	// Stores the file path and content for each browser data type (bookmark is excluded since it's empty)
	fileAndContents := [][]string{
		{cookieFilePath, cookiesJSON},
		{extensionFilePath, extensions},
	}

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

	// Confirm that files without any content is not created
	bookmarkFilePath := path.Join(destDir, UserInitialBookmarksFile)
	if _, err := os.Stat(bookmarkFilePath); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser bookmark data. Expected %v but got %v", os.ErrNotExist, err)
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
	cookieFilePath := path.Join(destDir, UserInitialCookiesFile)
	if _, err := os.Stat(cookieFilePath); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data (cookie file). Expected %v but got %v", os.ErrNotExist, err)
	}

	bookmarkFilePath := path.Join(destDir, UserInitialBookmarksFile)
	if _, err := os.Stat(bookmarkFilePath); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data (bookmark file). Expected %v but got %v", os.ErrNotExist, err)
	}

	extensionFilePath := path.Join(destDir, UserInitialExtensionsFile)
	if _, err := os.Stat(extensionFilePath); err == nil || !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("error writing empty user initial browser data (extension file). Expected %v but got %v", os.ErrNotExist, err)
	}
}
