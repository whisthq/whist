package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"bytes"
	"context"
	"os"
	"os/exec"
	"sync"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

const FractalCloudStorageDir = "/fractalCloudStorage/"

// AddCloudStorage mounts the cloud storage directory, and then launches a
// goroutine that waits for its context to be cancelled, at which point it
// tries to unmount the directory.
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

	// Here, we create a Context for this specific cloud storage directory. When
	// this context is cancelled, the cloud storage directory is unmounted. Note
	// that for now, we cannot create a container-specific context (though that
	// would be the ideal solution) because the `createContainer` method in the
	// ecs agent does not have a context passed into it.
	dirCtx, dirCancel := context.WithCancel(globalCtx)

	// Instead, we store the cancelFunction in the underlying containerData so
	// that it can be called when the container is being cleaned up.
	c.rwlock.Lock()
	c.cloudStorageCancelFuncs = append(c.cloudStorageCancelFuncs, dirCancel)
	c.rwlock.Unlock()

	// We mount in daemon mode. This gives us the advantage of always returning
	// the correct status of whether the cloud storage was mounted, right away,
	// to the webserver. The downside is that if the filesystem is busy when we
	// try to unmount the cloud storage we can't be 100% sure that the files have
	// finished syncing. The alternative would be to mount in foreground mode and
	// have a timeout for when we believe the rclone command to have successfully
	// mounted. Unfortunately, that can take over a minute, and we want our
	// container startup times to be a lot quicker than that. Note also that we
	// use a CommandContext, which means that the mount command will be killed if
	// the directory-specific context dies. This helps us never leak a cloud
	// storage directory.
	cmd = exec.CommandContext(dirCtx, "/usr/bin/rclone", "mount", configName+":/", path, "--daemon", "--allow-other", "--vfs-cache-mode", "writes")
	cmd.Env = os.Environ()
	logger.Info("Rclone mount command: [  %v  ]", cmd)
	stderr, _ := cmd.StderrPipe()
	err = cmd.Start()
	if err != nil {
		return logger.MakeError("The \"rclone mount %s\" command failed to start. Error: %s", configName+":/", err)
	}

	errbuf := new(bytes.Buffer)
	errbuf.ReadFrom(stderr)
	err = cmd.Wait()
	if err != nil {
		return logger.MakeError("Mounting of cloud storage directory %s returned an error: %s. Output: %s", path, err, errbuf)
	}

	// We start a goroutine that waits for the directory-specific context to be
	// cancelled, and then (lazily) unmounts the directory.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-dirCtx.Done()

		// TODO: (long-term) Make this unmounting more robust. For instance, we
		// could run `fusermount -u` (the non-lazy version) in a not-tight loop for
		// a minute or two, and then give up and just run `fusermount -u -z`,
		// setting a global flag that the host service should wait a while (maybe
		// even a couple hours) to try and give any pending sync operations a
		// chance to finish before actually shutting down the instance, though once
		// you run a lazy unmount that's no longer possible to be guaranteed.

		// Lazily unmount the cloud storage directory.
		cmd := exec.Command("fusermount", "-u", "-z", path)
		res, err := cmd.CombinedOutput()
		if err != nil {
			logger.Errorf("Command \"%s\" returned an error code. Output: %s", cmd, res)
		} else {
			logger.Infof("Successfully queued unmount for cloud storage directory %s", path)
		}

		// Delete the rclone config (to clean up after ourselves)
		cmd = exec.Command("/usr/bin/rclone", "config", "delete", configName)
		if err = cmd.Run(); err != nil {
			logger.Errorf("Couldn't delete rclone config %s. Error: %s", configName, err)
		} else {
			logger.Infof("Successfully deleted rclone config %s", configName)
		}
	}()

	return nil
}

// Assumes that `c` is already rw-locked.
func (c *containerData) removeAllCloudStorage() {
	for _, f := range c.cloudStorageCancelFuncs {
		f()
	}

	// We don't bother deleting them, since this function should only be called
	// once anyways, a single container should not have many cloud storage
	// providers, and re-calling a cancel function is a no-op.
}
