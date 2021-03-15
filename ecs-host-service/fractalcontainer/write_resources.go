// TODO: mark all packages with the import URL
package fractalcontainer

import (
	"os"
	"os/exec"
	"strings"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// TODO: revamp locking here

func (c *containerData) WriteResourcesForProtocol() error {
	var err error

	// Write identifying host port
	p, err := c.GetIdentifyingHostPort()
	if err != nil {
		return logger.MakeError("Couldn't write start values: %s", err)
	}
	err = writeDataToFile(c.getResourceMappingDir()+"hostPort_for_my_32262_tcp", logger.Sprintf("%d", p))
	if err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	// Write TTY
	err = writeDataToFile(c.getResourceMappingDir()+"tty", logger.Sprintf("%d", c.GetTTY()))
	if err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	return nil
}

func (c *containerData) WriteStartValues(dpi int, containerARN string) error {
	// Write DPI
	if err := writeDataToFile(c.getResourceMappingDir()+"DPI", logger.Sprintf("%v", dpi)); err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	// Write ContainerARN
	if err := writeDataToFile(c.getResourceMappingDir()+"ContainerARN", containerARN); err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	return nil
}

// Populate the config folder under the container's FractalID for the
// container's assigned user and running application.
func (c *containerData) PopulateUserConfigs() error {
	// Make directory for user configs
	configDir := c.getUserConfigDir()
	if err := os.MkdirAll(configDir, 0777); err != nil {
		return logger.MakeError("Could not make dir %s. Error: %s", configDir, err)
	}

	// TODO: makeFractalDirectoriesFreeForAll() ?

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
	tarPath := configPath + "fractal-app-config.tar.gz"
	untarConfigCmd := exec.Command("/usr/bin/tar", "-xzf", tarPath, "-C", configPath)
	untarConfigOutput, err := untarConfigCmd.CombinedOutput()
	if err != nil {
		logger.Errorf("Could not untar config archive: %s. Output: %s", err, untarConfigOutput)
	} else {
		logger.Infof("Untar config directory output: %s", untarConfigOutput)
	}

	return nil
}

func (c *containerData) BackupUserConfigs() error {
	// Clear contents of config directory at the end of the function
	defer c.cleanUserConfigDir()

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
	} else {
		logger.Infof("Tar config directory output: %s", tarConfigOutput)
	}

	saveConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", tarPath, s3ConfigPath)
	saveConfigOutput, err := saveConfigCmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Could not run \"aws s3 cp\" save config command: %s. Output: %s", err, saveConfigOutput)
	} else {
		logger.Infof("Ran \"aws s3 cp\" save config command with output: %s", saveConfigOutput)
	}

	return nil
}

func (c *containerData) cleanUserConfigDir() {
	err := os.RemoveAll(c.getUserConfigDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", c.getUserConfigDir(), err)
	}
}

func (c *containerData) getUserConfigDir() string {
	return logger.Sprintf("/fractal/%s/userConfigs/", c.fractalID)
}

func (c *containerData) getS3ConfigPath() string {
	return logger.Sprintf("s3://fractal-user-app-configs/%s/%s/fractal-app-config.tar.gz", c.userID, c.appName)
}

func (c *containerData) MarkReady() error {
	return writeDataToFile(c.getResourceMappingDir()+".ready", ".ready")
}

func (c *containerData) getResourceMappingDir() string {
	return logger.Sprintf("/fractal/%s/containerResourceMappings/", c.GetFractalID())
}

func (c *containerData) createResourceMappingDir() error {
	err := os.MkdirAll(c.getResourceMappingDir(), 0777)
	if err != nil {
		return logger.MakeError("Failed to create dir %s. Error: %s", c.getResourceMappingDir(), err)
	}
	return nil
}

func (c *containerData) cleanResourceMappingDir() {
	err := os.RemoveAll(c.getResourceMappingDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", c.getResourceMappingDir(), err)
	}
}

// TODO: refactor out all the `c.getResourceMappingDir() calls`
func writeDataToFile(filename, data string) (err error) {
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0777)
	if err != nil {
		return logger.MakeError("Unable to create file %s to store resource assignment. Error: %v", filename, err)
	}
	// Instead of deferring the close() and sync() of the file, as is
	// conventional, we do it at the end of the function to avoid some annoying
	// linter errors
	_, err = file.WriteString(data)
	if err != nil {
		return logger.MakeError("Couldn't write assignment with data %s to file %s. Error: %v", data, filename, err)
	}

	err = file.Sync()
	if err != nil {
		return logger.MakeError("Couldn't sync file %s. Error: %v", filename, err)
	}

	err = file.Close()
	if err != nil {
		return logger.MakeError("Couldn't close file %s. Error: %v", filename, err)
	}

	logger.Info("Wrote data \"%s\" to file %s\n", data, filename)
	return nil
}
