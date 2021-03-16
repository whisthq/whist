// TODO: mark all packages with the import URL
package fractalcontainer

import (
	"os"

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
	err = c.writeResourceMappingToFile("hostPort_for_my_32262_tcp", logger.Sprintf("%d", p))
	if err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write TTY
	err = c.writeResourceMappingToFile("tty", logger.Sprintf("%d", c.GetTTY()))
	if err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	return nil
}

func (c *containerData) WriteStartValues(dpi int, containerARN string) error {
	// Write DPI
	if err := c.writeResourceMappingToFile("DPI", logger.Sprintf("%v", dpi)); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write ContainerARN
	if err := c.writeResourceMappingToFile("ContainerARN", containerARN); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	return nil
}

func (c *containerData) MarkReady() error {
	return c.writeResourceMappingToFile(".ready", ".ready")
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

func (c *containerData) writeResourceMappingToFile(filename, data string) (err error) {
	file, err := os.OpenFile(c.getResourceMappingDir()+filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0777)
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
