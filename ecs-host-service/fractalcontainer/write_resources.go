// TODO: mark all packages with the import URL
package fractalcontainer

import (
	"os"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// TODO: revamp locking here

func (c *containerData) WriteResourcesForProtocol() error {
	var err error

	// Write identifying host port
	p, err := c.getIdentifyingHostPort()
	if err != nil {
		return logger.MakeError("Couldn't write start values: %s", err)
	}
	err = writeDataToFile(c.getResourceMappingDir()+"hostPort_for_my_32262_tcp", logger.Sprintf("%d", p))
	if err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	err = writeDataToFile(c.getResourceMappingDir()+"tty", logger.Sprintf("%d", c.GetTTY()))
	if err != nil {
		// Don't need to wrap err here because writeDataToFile already contains the relevant info
		return err
	}

	return nil
}

func (c *containerData) WriteStartValues() error {
	// TODO
	return nil
}

func (c *containerData) MarkReady() error {
	return writeDataToFile(c.getResourceMappingDir()+".ready", ".ready")
}

func (c *containerData) getIdentifyingHostPort() (uint16, error) {
	binds := c.GetPortBindings()
	for _, b := range binds {
		if b.Protocol == portbindings.TransportProtocolTCP && b.ContainerPort == 32262 {
			return b.HostPort, nil
		}
	}

	return 0, logger.MakeError("Couldn't getIdentifyingHostPort() for container with FractalID %s", c.GetFractalID())
}

func (c *containerData) getResourceMappingDir() string {
	return logger.Sprintf("/fractal/%s/containerResourceMappings/", c.GetFractalID())
}

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
