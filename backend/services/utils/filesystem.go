package utils // import "github.com/whisthq/whist/backend/services/utils"

import (
	"context"
	"errors"
	"log"
	"os"
	"path"
	"time"

	"github.com/fsnotify/fsnotify"
)

// WaitForFileCreation blocks until the provided filename is created in the
// provided directory, or the timeout duration elapses. If the target file is
// created in time, a nil error is returned. If the timeout elapses, a
// context.DeadlineExceeded error is returned. In any other case, a non-nil
// error is returned explaining what went wrong.
//
// For maximum correctness, we require that any paths passed in are absolute.
// This may not be strictly necessary, but the documentation for `fsnotify` and
// `path/filepath` are just vague enough for me to enforce this rule for the
// callers. This rule may be subject to relaxation in the future.
//
// The function accepts a pointer to a fsnotify watcher. If the caller passes in
// nil then we will create a new watcher and handle the clean up. If a watcher
// is passed by the caller then they are expected to clean up their watcher.
//
// NOTE: Each invocation of this function creates an `inotify` instance and
// holds onto it for the duration of this function call. By default, our
// instances have a limit of 128 watchers per user. Therefore, we bump this
// limit in host-setup to prevent it from being a limiting factor for our
// mandelbox launches.
func WaitForFileCreation(absParentDirectory, fileName string, timeout time.Duration, watcher *fsnotify.Watcher) error {
	if !path.IsAbs(absParentDirectory) {
		return MakeError("Can't pass non-absolute paths into WaitForFileCreation")
	}
	targetFileName := path.Join(absParentDirectory, fileName)

	var err error
	if watcher == nil {
		watcher, err = fsnotify.NewWatcher()
		if err != nil {
			return MakeError("Couldn't create new fsnotify.Watcher: %s", err)
		}
		defer watcher.Close()
	}

	// Check if the file has already been created before watcher watches for file
	if _, err := os.Stat(targetFileName); err == nil {
		return nil
	}

	if err = watcher.Add(absParentDirectory); err != nil {
		return MakeError("Error adding dir %s to fsnotify.Watcher: %s", absParentDirectory, err)
	}

	// Check if the file has already been created before watcher watches for file
	if _, err := os.Stat(targetFileName); err == nil {
		return nil
	}

	if err = waitForErrorOrCreation(timeout, targetFileName, watcher.Events, watcher.Errors); err != nil {
		return MakeError("Error waiting for file creation: %v", err)
	}

	return nil
}

// waitForErrorOrCreation will handle watcher events, errors, and timeouts that occur
func waitForErrorOrCreation(timeout time.Duration, targetFileName string, watcherEvent chan fsnotify.Event, watcherErr chan error) error {
	// Create timer here
	timer := time.NewTimer(timeout)

	for {
		select {
		case <-timer.C:
			return context.DeadlineExceeded

		case err, ok := <-watcherErr:
			if !ok {
				return MakeError("fsnotify.Watcher error channel closed with error: %v", err)
			}
			// Note that for us, dropped events _are_ errors, since we should not be
			// having nearly enough filesystem activity to drop any events.
			return MakeError("returned error: %s", err)

		case ev, ok := <-watcherEvent:
			if !ok {
				return MakeError("fsnotify.Watcher events channel closed.")
			}
			// TODO: remove this log once we're confident this part of the code works
			log.Printf("fsnotify.Watched filesystem event: %+v", ev)
			// Check if it's a creation event that matches the filename we expect
			if ev.Op&fsnotify.Create == fsnotify.Create && ev.Name == targetFileName {
				return nil
			}
		}
	}
}

// writeToNewFile creates a file then writes the content
func WriteToNewFile(filePath string, content string) error {
	// If file already exists return an error
	_, err := os.Stat(filePath)
	if err == nil {
		return MakeError("Could not make file %s because it already exists!", filePath)
	} else if !errors.Is(err, os.ErrNotExist) {
		return MakeError("Could not make file %s. Error: %s", filePath, err)
	}

	newFile, err := os.OpenFile(filePath, os.O_CREATE|os.O_WRONLY, 0777)
	if err != nil {
		return MakeError("Could not make file %s. Error: %s", filePath, err)
	}

	newFile.WriteString(content)

	return nil
}
