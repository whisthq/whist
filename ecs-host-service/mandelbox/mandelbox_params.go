package mandelbox // import "github.com/fractal/fractal/ecs-host-service/mandelbox"

import (
	"os"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	"github.com/fractal/fractal/ecs-host-service/utils"
)

func (c *mandelboxData) WriteMandelboxParams() error {
	var err error

	// Write identifying host port
	p, err := c.GetIdentifyingHostPort()
	if err != nil {
		return utils.MakeError("Couldn't write mandelbox params: %s", err)
	}
	if err = c.writeResourceMappingToFile("hostPort_for_my_32262_tcp", utils.Sprintf("%d", p)); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write TTY. Note that we use `c.GetTTY()` instead of `c.tty` for the
	// locking.
	if err := c.writeResourceMappingToFile("tty", utils.Sprintf("%d", c.GetTTY())); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write GPU Index. Note that we use `c.GetGPU()` instead of `c.tty` for
	// the locking.
	if err := c.writeResourceMappingToFile("gpu_index", utils.Sprintf("%d", c.GetGPU())); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	return nil
}

func (c *mandelboxData) WriteProtocolTimeout(seconds int) error {
	// Write timeout
	if err := c.writeResourceMappingToFile("timeout", utils.Sprintf("%v", seconds)); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}
	return nil
}

func (c *mandelboxData) MarkReady() error {
	return c.writeResourceMappingToFile(".ready", ".ready")
}

func (c *mandelboxData) getResourceMappingDir() string {
	return utils.Sprintf("%s%s/mandelboxResourceMappings/", utils.FractalDir, c.mandelboxID)
}

func (c *mandelboxData) createResourceMappingDir() error {
	err := os.MkdirAll(c.getResourceMappingDir(), 0777)
	if err != nil {
		return utils.MakeError("Failed to create dir %s. Error: %s", c.getResourceMappingDir(), err)
	}
	return nil
}

func (c *mandelboxData) cleanResourceMappingDir() {
	err := os.RemoveAll(c.getResourceMappingDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", c.getResourceMappingDir(), err)
	}
}

func (c *mandelboxData) writeResourceMappingToFile(filename, data string) (err error) {
	file, err := os.OpenFile(c.getResourceMappingDir()+filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0777)
	if err != nil {
		return utils.MakeError("Unable to create file %s to store resource assignment. Error: %v", filename, err)
	}
	// Instead of deferring the close() and sync() of the file, as is
	// conventional, we do it at the end of the function to avoid some annoying
	// linter errors
	_, err = file.WriteString(data)
	if err != nil {
		return utils.MakeError("Couldn't write assignment with data %s to file %s. Error: %v", data, filename, err)
	}

	err = file.Sync()
	if err != nil {
		return utils.MakeError("Couldn't sync file %s. Error: %v", filename, err)
	}

	err = file.Close()
	if err != nil {
		return utils.MakeError("Couldn't close file %s. Error: %v", filename, err)
	}

	logger.Info("Wrote data \"%s\" to file %s\n", data, filename)
	return nil
}
