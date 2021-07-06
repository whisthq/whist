package utils // import "github.com/fractal/fractal/ecs-host-service/utils"

import (
	"context"
	"log"
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
// NOTE: Each invocation of this function creates an `inotify` instance and
// holds onto it for the duration of this function call. By default, our
// instances have a limit of 128 watchers per user. Therefore, we bump this
// limit in ecs-host-setup to prevent it from being a limiting factor for our
// mandelbox launches.
func WaitForFileCreation(absParentDirectory, fileName string, timeout time.Duration) error {
	if !path.IsAbs(absParentDirectory) {
		return MakeError("Can't pass non-absolute paths into WaitForFileCreation")
	}
	targetFileName := path.Join(absParentDirectory, fileName)

	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		return MakeError("Couldn't create new fsnotify.Watcher: %s", err)
	}
	defer watcher.Close()

	err = watcher.Add(absParentDirectory)
	if err != nil {
		return MakeError("Error adding dir %s to fsnotify.Watcher: %s", absParentDirectory, err)
	}

	timer := time.NewTimer(timeout)

	for {
		select {
		case <-timer.C:
			return context.DeadlineExceeded

		case err, ok := <-watcher.Errors:
			if !ok {
				return MakeError("fsnotify.Watcher error channel closed.")
			}
			// Note that for us, dropped events _are_ errors, since we should not be
			// having nearly enough filesystem activity to drop any events.
			return MakeError("fsnotify returned error: %s", err)

		case ev, ok := <-watcher.Events:
			if !ok {
				return MakeError("fsnotify.Watcher events channel closed.")
			}
			// TODO: remove this log once we're confident this part of the code works
			log.Printf("Watched filesystem event: %+v", ev)
			// Check if it's a creation event that matches the filename we expect
			if ev.Op&fsnotify.Create == fsnotify.Create && ev.Name == targetFileName {
				return nil
			}
		}
	}

}
