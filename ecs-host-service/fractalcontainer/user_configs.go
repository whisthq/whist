package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"os"
	"os/exec"
	"strings"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

func (c *containerData) PopulateUserConfigs() error {
	// We (write) lock here so that the resourceMappingDir doesn't get cleaned up
	// from under us.
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	// Make directory for user configs
	configDir := c.getUserConfigDir()
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return logger.MakeError("Could not make dir %s. Error: %s", configDir, err)
	}

	// Once we've extracted everything, we open up permissions for the user
	// config directory so it's accessible by the non-root user in the
	// containers. We also are okay with setting executable bits here, since it's
	// possible that some apps store executable scripts in their config.
	defer func() {
		cmd := exec.Command("chown", "-R", "ubuntu", configDir)
		cmd.Run()
		cmd = exec.Command("chmod", "-R", "777", configDir)
		cmd.Run()
	}()

	// If userID is not set, then we don't retrieve configs from s3
	if len(c.userID) == 0 {
		return nil
	}

	// Retrieve config from s3
	s3ConfigPath := c.getS3ConfigPath()
	getConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", s3ConfigPath, configDir)
	getConfigOutput, err := getConfigCmd.CombinedOutput()
	// If aws s3 cp errors out due to the file not existing, don't log an error because
	//    this means that it's the user's first run and they don't have any settings
	//    stored for this application yet.
	if err != nil {
		if strings.Contains(string(getConfigOutput), "does not exist") {
			logger.Infof("Ran \"aws s3 cp\" and config does not exist")
			return nil
		}
		return logger.MakeError("Could not run \"aws s3 cp\" get config command: %s. Output: %s", err, getConfigOutput)
	}

	logger.Infof("Ran \"aws s3 cp\" get config command with output: %s", getConfigOutput)

	// Extract the config archive to the user config directory
	tarPath := configDir + "fractal-app-config.tar.gz"
	untarConfigCmd := exec.Command("/usr/bin/tar", "-xzf", tarPath, "-C", configDir)
	untarConfigOutput, err := untarConfigCmd.CombinedOutput()
	if err != nil {
		logger.Errorf("Could not untar config archive: %s. Output: %s", err, untarConfigOutput)
	} else {
		logger.Infof("Untar config directory output: %s", untarConfigOutput)
	}

	return nil
}

// This function requires that `c.rwlock()` is locked. It backs up the user
// config to s3.
func (c *containerData) backupUserConfigs() error {
	if len(c.userID) == 0 {
		return logger.MakeError("Cannot save user configs for FractalID %s since UserID is empty.", c.fractalID)
	}

	configDir := c.getUserConfigDir()
	tarPath := configDir + "fractal-app-config.tar.gz"
	s3ConfigPath := c.getS3ConfigPath()

	tarConfigCmd := exec.Command("/usr/bin/tar", "-C", configDir, "-czf", tarPath, "--exclude=fractal-app-config.tar.gz", ".")
	tarConfigOutput, err := tarConfigCmd.CombinedOutput()
	// tar is only fatal when exit status is 2 -
	//    exit status 1 just means that some files have changed while tarring,
	//    which is an ignorable error
	if err != nil && !strings.Contains(string(tarConfigOutput), "file changed") {
		return logger.MakeError("Could not tar config directory: %s. Output: %s", err, tarConfigOutput)
	}
	logger.Infof("Tar config directory output: %s", tarConfigOutput)

	saveConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", tarPath, s3ConfigPath)
	saveConfigOutput, err := saveConfigCmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Could not run \"aws s3 cp\" save config command: %s. Output: %s", err, saveConfigOutput)
	}

	logger.Infof("Ran \"aws s3 cp\" save config command with output: %s", saveConfigOutput)
	return nil
}

// This function requires that `c.rwlock()` is locked. It cleans up the user
// config directory for a container.
func (c *containerData) cleanUserConfigDir() {
	err := os.RemoveAll(c.getUserConfigDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", c.getUserConfigDir(), err)
	}
}

func (c *containerData) getUserConfigDir() string {
	return logger.Sprintf("%s%s/userConfigs/", logger.FractalDir, c.fractalID)
}

func (c *containerData) getS3ConfigPath() string {
	return logger.Sprintf("s3://fractal-user-app-configs/%s/%s/fractal-app-config.tar.gz", c.userID, c.appName)
}
