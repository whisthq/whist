package utils

import (
	"bytes"
	"io/ioutil"
	"os"
	"path"
	"testing"
	"time"

	"github.com/fsnotify/fsnotify"
)

// TestWaitForFileCreationNotAbsolute will verify the function requires an absolute directory path
func TestWaitForFileCreationNotAbsolute(t *testing.T) {
	newWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		t.Fatalf("Couldn't create new fsnotify.Watcher: %s", err)
	}
	defer newWatcher.Close()
	// WaitForFileCreation expects an absolute path to the directory and will return an error
	if err := WaitForFileCreation("not-abs-dir-path/", "test", time.Second*1, newWatcher); err == nil {
		t.Fatal("error waiting for file creation when path is not absolute. Expected err, got nil")
	}
}

//TestWaitForFileCreationWatcherClosed will verify if a closed watcher are caught
func TestWaitForFileCreationWatcherClosed(t *testing.T) {
	newWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		t.Fatalf("Couldn't create new fsnotify.Watcher: %s", err)
	}
	newWatcher.Close()
	// WaitForFileCreation will encounter an issue adding parent directory and will return an error
	if err := WaitForFileCreation("/", "test", time.Second*1, newWatcher); err == nil {
		t.Fatal("error waiting for file creation when watcher is closed. Expected err, got nil")
	}
}

// TestWaitForFileCreationWatchError verify watcher errors are caught
func TestWaitForFileCreationWatchError(t *testing.T) {
	newWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		t.Fatalf("Couldn't create new fsnotify.Watcher: %s", err)
	}

	defer newWatcher.Close()
	go func() {
		newWatcher.Errors <- MakeError("test watch error")
	}()

	// WaitForFileCreation weill handle the watcher error being closed
	if err := WaitForFileCreation("/", "test", time.Second*1, newWatcher); err == nil {
		t.Fatal("error waiting for file creation when watcher chan has error. Expected err, got nil")
	}
}

// TestWaitForFileCreationWatchErrorClosed will verify watcher error chan closed are caught
func TestWaitForFileCreationWatchErrorClosed(t *testing.T) {
	newWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		t.Fatalf("Couldn't create new fsnotify.Watcher: %s", err)
	}

	close(newWatcher.Errors)

	// WaitForFileCreation will return an error because watcher's error channel is closed
	if err := WaitForFileCreation("/", "test", time.Second*1, newWatcher); err == nil {
		t.Fatal("error waiting for file creation when watcher error chan is closed. Expected err, got nil")
	}
	
	// We closed the chan but newWatcher.Close() will attempt to close it again so we add the chan back
	newWatcher.Errors = make(chan error)
	newWatcher.Close()
}

// TestWaitForFileCreationWatchEventClosed will handle the watcher event chan being closed
func TestWaitForFileCreationWatchEventClosed(t *testing.T) {
	newWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		t.Fatalf("Couldn't create new fsnotify.Watcher: %s", err)
	}

	close(newWatcher.Events)

	// WaitForFileCreation will return an error because watcher's event channel is closed
	if err := WaitForFileCreation("/", "test", time.Second*1, newWatcher); err == nil {
		t.Fatal("error waiting for file creation when watcher event chan is closed. Expected err, got nil")
	}

	// We closed the chan but newWatcher.Close() will attempt to close it again so we add the chan back
	newWatcher.Events = make(chan fsnotify.Event)
	newWatcher.Close()
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

// TestWriteToNewFileMissingDirectory will lead to an error creating the file as directory will not exist
func TestWriteToNewFileMissingDirectory(t *testing.T) {
	// Create directory
	destDir, err := ioutil.TempDir("", "testWriteToNewFile")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(destDir)

	filePath := path.Join(destDir, "non_existent_dir/testingFile")
	testFileContent := "test content"

	// WriteToNewFile should succeed in writing to new file
	if err := WriteToNewFile(filePath, testFileContent); err == nil {
		t.Fatalf("error writing to new file %s with missing directory. Expected err, got nil", filePath)
	}
}
