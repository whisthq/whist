package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"bytes"
	"context"
	"os"
	"os/exec"
	"sync"
	"time"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

const FractalCloudStorageDir = "/fractalCloudStorage/"

// AddCloudStorage (at a very high level) launches a goroutine that mounts the
// cloud storage directory and waits around to clean it up once it's unmounted.
func (c *containerData) AddCloudStorage(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, Provider cloudstorage.Provider, AccessToken string, RefreshToken string, Expiry string, TokenType string, ClientID string, ClientSecret string) error {
	rcloneToken, err := cloudstorage.GenerateRcloneToken(AccessToken, RefreshToken, Expiry, TokenType)
	if err != nil {
		return logger.MakeError("Couldn't add cloud storage: %s", err)
	}

	providerRcloneName, err := Provider.RcloneInternalName()
	if err != nil {
		return logger.MakeError("Couldn't add cloud storage: %s", err)
	}

	providerPrettyName, err := Provider.PrettyName()
	if err != nil {
		return logger.MakeError("Couldn't add cloud storage: %s", err)
	}

	configName := providerRcloneName + "-" + string(c.fractalID)

	err = cloudstorage.CreateRcloneConfig(configName, Provider, rcloneToken, ClientID, ClientSecret)
	if err != nil {
		return logger.MakeError("Couldn't add cloud storage provider: %s", err)
	}

	// Don't forget the trailing slash
	path := logger.Sprintf("%s%s/%s/", FractalCloudStorageDir, c.fractalID, providerPrettyName)

	// Make directory to mount in
	err = os.MkdirAll(path, 0777)
	if err != nil {
		return logger.MakeError("Could not mkdir path %s. Error: %s", path, err)
	}
	logger.Infof("Created directory %s", path)

	// Fix cloud storage directory permissions
	// TODO: this could probably be made more efficient, but we will revisit this
	// once we're actually using cloud storage again.
	cmd := exec.Command("chown", "-R", "ubuntu", FractalCloudStorageDir)
	cmd.Run()
	cmd = exec.Command("chmod", "-R", "666", FractalCloudStorageDir)
	cmd.Run()

	// Here, we create a Context and CancelFunc for this specific cloud storage
	// directory. During normal operation, when this context is cancelled, the
	// cloud storage directory is (lazily) unmounted.
	dirCtx, dirCancel := context.WithCancel(globalCtx)

	// Now, we start a goroutine that kicks off the cloud storage mounting
	// process (in yet another goroutine). We need a whole goroutine here, just
	// to start another one, because we want to wait for some given amount of
	// time without an error before assuming the mount command succeeded, but
	// don't want to block the main thread. Once the  timeout period has elapsed,
	// we return `nil` to indicate a (most likely) successful mount.

	// We use resultChan to report the error or success of the mount command.
	resultChan := make(chan error)

	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Now, we actually start the goroutine that runs the mount command. That
		// goroutine is free to block on the mount command, and it does so,
		// cleaning up once the mount command has exited. The inner goroutine
		// communicates with the current goroutine using `innerChan`. Note that we
		// cannot just use `resultChan` directly since we need to notify _both_
		// parent goroutines, and we can't safely have both parents listening on
		// one channel and guarantee that we "pop the stack" of goroutines in the
		// correct order.
		innerChan := make(chan error)

		goroutineTracker.Add(1)
		go func() {
			defer goroutineTracker.Done()
			defer close(innerChan)

			// We mount in foreground mode, and wait for the result to clean up the
			// directory created for this purpose. That way we know that we aren't
			// accidentally removing files from the user's cloud storage drive. Note
			// also that we are NOT using a `exec.CommandContext`, since if the
			// context is cancelled the rclone mounting command would be killed,
			// which may not actually be a good thing (TODO: test this out). Instead,
			// we'd rather let the mounting finish and then run the unmounting. This
			// can potentially slow down the host service shutdown process, but that
			// is a small price to pay to avoid messing up users' cloud storage.
			cmd = exec.Command("/usr/bin/rclone", "mount", configName+":/", path, "--allow-other", "--vfs-cache-mode", "writes")
			cmd.Env = os.Environ()
			logger.Info("Rclone mount command: [  %v  ]", cmd)
			stderr, _ := cmd.StderrPipe()

			// After the mount command returns (which means the cloud storage is no
			// longer mounted), we remove the now-unnecessary directory we created.
			// Note that we don't use `os.RemoveAll()` just in case there's something
			// in the directory, since that would delete files from the user's cloud
			// storage.
			defer func() {
				remerr = os.Remove(path)
				if remerr != nil {
					logger.Errorf("Error removing cloud storage directory %s: %s", path, err)
				}
			}()	

			// Actually start the mount.
			err = cmd.Start()
			if err != nil {
				innerChan <- logger.MakeError("Could not start \"rclone mount %s\" command: %s", configName+":/", err)
			} 
			// If there was no error, then innerChan gets closed without an error
			// being sent.

			// We have to call dirCancel() here in case the mount failed with an
			// error *after* the enclosing goroutine's timeout already passed, so the
			// AddCloudStorage() function reported a 
			dirCancel()
		}() 

	}()

	//

	//

		c.cloudStorageDirectories = append(c.cloudStorageDirectories, path)


		// We close errorchan after `timeout` so the enclosing function can return
		// an error if the `rclone mount` command failed immediately, or return nil
		// if it didn't.
		timeout := time.Second * 15
		timer := time.AfterFunc(timeout, func() { close(errorchan) })
		logger.Infof("Attempting to mount storage directory %s", path)

		errbuf := new(bytes.Buffer)
		errbuf.ReadFrom(stderr)

		err = cmd.Wait()
		if err != nil {
			errorchanStillOpen := timer.Stop()
			if errorchanStillOpen {
				errorchan <- logger.MakeError("Mounting of cloud storage directory %s returned an error: %s. Output: %s", path, err, errbuf)
				close(errorchan)
			} else {
				logger.Errorf("Mounting of cloud storage directory %s failed after more than timeout %v and was therefore not reported as a failure to the webserver. Error: %s. Output: %s", path, timeout, err, errbuf)
			}
		}
	}

}

func (c *containerData) RemoveAllCloudStorage() {
	for _, path := range c.cloudStorageDirectories {
		// Unmount lazily, i.e. will unmount as soon as the directory is not busy
		cmd := exec.Command("fusermount", "-u", "-z", path)
		res, err := cmd.CombinedOutput()
		if err != nil {
			logger.Errorf("Command \"%s\" returned an error code. Output: %s", cmd, res)
		} else {
			logger.Infof("Successfully queued unmount for cloud storage directory %s", path)
		}
	}

	c.cloudStorageDirectories = []string{}
}
