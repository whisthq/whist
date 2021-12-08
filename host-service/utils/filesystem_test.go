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
	// WaitForFileCreation expects an absolute path to the directory and will return an error
	if err := WaitForFileCreation("not-abs-dir-path/", "test", time.Second*1, nil); err == nil {
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

// TestWaitForErrorOrCreationErrorChan verify watcher errors are caught
func TestWaitForErrorOrCreationErrorChan(t *testing.T) {
	watcherEvents := make(chan fsnotify.Event)
	watcherErr := make(chan error)
	
	go func() {
		watcherErr <- MakeError("test watch error")
	}()

	defer close(watcherEvents)
	defer close(watcherErr)

	// waitForErrorOrCreation will return an error because the error chan received an error
	if err := waitForErrorOrCreation(time.Second*10, "targetFileName", watcherEvents, watcherErr); err == nil {
		t.Fatal("error waiting for file creation when watcher chan has error. Expected err, got nil")
	}
}

// TestWaitForErrorOrCreationErrorChanClosed will handle the error chan being closed
func TestWaitForErrorOrCreationErrorChanClosed(t *testing.T) {
	watcherEvents := make(chan fsnotify.Event)
	watcherErr := make(chan error)

	defer close(watcherEvents)
	close(watcherErr)

	// waitForErrorOrCreation will return an error because the event channel is closed
	if err := waitForErrorOrCreation(time.Second*10, "targetFileName", watcherEvents, watcherErr); err == nil {
		t.Fatal("error waiting for file creation when error chan is closed. Expected err, got nil")
	}
}

// TestWaitForErrorOrCreationEventChanClosed will handle the events chan being closed
func TestWaitForErrorOrCreationEventChanClosed(t *testing.T) {
	watcherEvents := make(chan fsnotify.Event)
	watcherErr := make(chan error)

	close(watcherEvents)
	defer close(watcherErr)

	// waitForErrorOrCreation will return an error because watcher's event channel is closed
	if err := waitForErrorOrCreation(time.Second*10, "targetFileName", watcherEvents, watcherErr); err == nil {
		t.Fatal("error waiting for file creation when event chan is closed. Expected err, got nil")
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
