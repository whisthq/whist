package fractalcontainer

import (
	"bytes"
	"os"
	"os/exec"
	"time"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

const fractalCloudStorageDir = "/fractalCloudStorage/"

// Mounts the cloud storage directory and waits around to clean it up once it's
// unmounted.
func (c *containerData) AddCloudStorage(Provider cloudstorage.Provider, AccessToken string, RefreshToken string, Expiry string, TokenType string, ClientID string, ClientSecret string) error {
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
	path := logger.Sprintf("%s%s/%s/", fractalCloudStorageDir, c.fractalID, providerPrettyName)

	// Make directory to mount in
	err = os.MkdirAll(path, 0777)
	if err != nil {
		return logger.MakeError("Could not mkdir path %s. Error: %s", path, err)
	}
	logger.Infof("Created directory %s", path)

	// Fix cloud storage directory permissions
	// TODO: this could probably be made more efficient, but we will revisit this
	// once we're actually using cloud storage.
	cmd := exec.Command("chown", "-R", "ubuntu", fractalCloudStorageDir)
	cmd.Run()
	cmd = exec.Command("chmod", "-R", "666", fractalCloudStorageDir)
	cmd.Run()

	// Mount in separate goroutine so we don't block the main goroutine.
	// Synchronize using errorchan.
	errorchan := make(chan error)
	go func() {
		// We mount in foreground mode, and wait for the result to clean up the
		// directory created for this purpose. That way we know that we aren't
		// accidentally removing files from the user's cloud storage drive.
		cmd = exec.Command("/usr/bin/rclone", "mount", configName+":/", path, "--allow-other", "--vfs-cache-mode", "writes")
		cmd.Env = os.Environ()
		logger.Info("Rclone mount command: [  %v  ]", cmd)
		stderr, _ := cmd.StderrPipe()

		c.cloudStorageDirectories = append(c.cloudStorageDirectories, path)

		err = cmd.Start()
		if err != nil {
			errorchan <- logger.MakeError("Could not start \"rclone mount %s\" command: %s", configName+":/", err)
			close(errorchan)
			return
		}

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

		// Remove the now-unnecessary directory we created. Note that we don't use
		// `os.RemoveAll()` in case there's something in the directory.
		err = os.Remove(path)
		if err != nil {
			logger.Errorf("Error removing cloud storage directory %s: %s", path, err)
		}
	}()

	err = <-errorchan
	return err
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
