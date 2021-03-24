package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"bytes"
	"os"
	"os/exec"
	"sync"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// FractalCloudStorageDir is the directory on the host that contains all cloud
// storage directories inside it. Note that we keep the cloud storage in its
// own directory, not in the logger.FractalDir, so that we can safely delete
// the entire `fractal` directory on exit.
const FractalCloudStorageDir = "/fractalCloudStorage/"

// AddCloudStorage mounts the cloud storage directory, and then launches a
// goroutine that waits for the container's context to be cancelled, at which
// point it tries to unmount the directory.
func (c *containerData) AddCloudStorage(goroutineTracker *sync.WaitGroup, Provider cloudstorage.Provider, AccessToken string, RefreshToken string, Expiry string, TokenType string, ClientID string, ClientSecret string) error {
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

	// Fix cloud storage directory permissions. Note that we don't need to set
	// the permissions to 0666, since we do that explicitly in the rclone mount
	// command below.
	//
	// TODO: this could probably be made more efficient using the `uid`, `gid`,
	// and `umask` options for `rclone`, but we will revisit this once we're
	// actually using cloud storage again.
	cmd := exec.Command("chown", "-R", "ubuntu", FractalCloudStorageDir)
	cmd.Run()

	// We mount in daemon mode. This gives us the advantage of always returning
	// the correct status of whether the cloud storage was mounted, right away,
	// to the webserver. The downside is that if the filesystem is busy when we
	// try to unmount the cloud storage we can't be 100% sure that the files have
	// finished syncing. The alternative would be to mount in foreground mode and
	// have a timeout for when we believe the rclone command to have successfully
	// mounted. Unfortunately, that can take over a minute, and we want our
	// container startup times to be a lot quicker than that. Note also that we
	// use a CommandContext, which means that the mount command will be killed if
	// the container-specific context dies. This helps us never leak a cloud
	// storage directory.
	cmd = exec.CommandContext(c.ctx,
		"/usr/bin/rclone", "mount", configName+":/", path,
		"--daemon",
		"--allow-other",
		"--vfs-cache-mode", "writes",
		"--dir-perms", "0777",
		"--file-perms", "0666",
		"--umask", "0",
	)
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

	// We start a goroutine that waits for the container-specific context to be
	// cancelled, and then (lazily) unmounts the directory.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-c.ctx.Done()

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
