package utils

import (
	"bytes"
	"io/ioutil"
	"os"
	"path"
	"testing"
	"time"
)

// TODO (aaron): Figure out how to test watcher related errors

// TestWaitForFileCreationNotAbsolute will verify the function requires an absolute directory path
func TestWaitForFileCreationNotAbsolute(t *testing.T) {
	// WaitForFileCreation expects an absolute path to the directory and will return an error
	if err := WaitForFileCreation("not-abs-dir-path/", "test", time.Second*1); err == nil {
		t.Fatal("error waiting for file creation when path is not absolute. Expected err, got nil")
	}
}

// TestWriteToNewFile  verify a file has been created
func TestWriteToNewFile(t *testing.T) {
	// Create directory
	destDir, err := ioutil.TempDir("", "testWriteToNewFile")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(destDir)

	filePath := path.Join(destDir, "testingFile")
	testFileContent := "test content"

	// WriteToNewFile should succeed in writing to new file
	if err := WriteToNewFile(filePath, testFileContent); err != nil {
		t.Fatalf("error writing to new file %s. Expected nil, got %v", filePath, err)
	}

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
		t.Errorf("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContent, matchingFileBuf.String())
	}

}

// TestWriteToNewFileButFileExists should return error on second time writing to new file
func TestWriteToNewFileButFileExistsFile(t *testing.T) {
	// Create directory
	destDir, err := ioutil.TempDir("", "testWriteToNewFile")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(destDir)

	filePath := path.Join(destDir, "testingFile")
	testFileContent := "test content"

	// WriteToNewFile should succeed in writing to new file
	if err := WriteToNewFile(filePath, testFileContent); err != nil {
		t.Fatalf("error writing to new file %s. Expected nil, got %v", filePath, err)
	}

	// WriteToNewFile should overwrite the file as the file exists
	if err := WriteToNewFile(filePath, testFileContent); err == nil {
		t.Fatalf("error writing to file that already exists %s. Expected err, got nil", filePath)
	}	
}
