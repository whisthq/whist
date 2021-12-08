package mandelbox

import (
	"bytes"
	"errors"
	"os"
	"io/ioutil"
	"path"
	"testing"
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

	cookieJSON := "[" + testCookie1 + "," + testCookie2 + "]"
	bookmarksJSON := ""

	// Create browser data
	userInitialBrowserData := BrowserData{
		CookiesJSON: cookieJSON,
		BookmarksJSON: bookmarksJSON,
	}

	if err := WriteUserInitialBrowserData(userInitialBrowserData, destDir); err != nil {
		t.Fatalf("error writing user initial browser data: %v", err)
	}

	// Get browser data file path
	cookieFilePath := path.Join(destDir, UserInitialCookiesFile)
	bookmarkFilePath := path.Join(destDir, UserInitialBookmarksFile)

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
}
