package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

import (
	"os"

	types "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

func (mandelbox *mandelboxData) WriteMandelboxParams() error {
	var err error

	// Write identifying host port
	p, err := mandelbox.GetIdentifyingHostPort()
	if err != nil {
		return utils.MakeError("Couldn't write mandelbox params: %s", err)
	}
	if err = mandelbox.writeResourceMappingToFile("hostPort_for_my_32262_tcp", utils.Sprintf("%d", p)); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write TTY. Note that we use `mandelbox.GetTTY()` instead of `mandelbox.tty` for the
	// locking.
	if err := mandelbox.writeResourceMappingToFile("tty", utils.Sprintf("%d", mandelbox.GetTTY())); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write GPU Index. Note that we use `mandelbox.GetGPU()` instead of `mandelbox.gpuIndex` for
	// the locking.
	if err := mandelbox.writeResourceMappingToFile("gpu_index", utils.Sprintf("%d", mandelbox.GetGPU())); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	// Write Session ID. Note that we use `mandelbox.GetSessionID()` instead of `mandelbox.sessionID` for
	// the locking.
	if err := mandelbox.writeResourceMappingToFile("session_id", utils.Sprintf("%v", mandelbox.GetSessionID())); err != nil {
		// Don't need to wrap err here because it already contains the relevant info
		return err
	}

	return nil
}

func (mandelbox *mandelboxData) WriteProtocolTimeout(seconds int) error {
	return mandelbox.writeResourceMappingToFile("timeout", utils.Sprintf("%v", seconds))
}

// WriteJSONData writes the data received through JSON transport
// to the config.json file located on the resourceMappingDir.
func (mandelbox *mandelboxData) WriteJSONData(data types.JSONData) error {
	return mandelbox.writeResourceMappingToFile("config.json", string(data))
}

// MarkParamsReady indicates that the mandelbox's services that do NOT depend
// on s3 userConfigs (e.g. audio and video services) can be started
func (mandelbox *mandelboxData) MarkParamsReady() error {
	return mandelbox.writeResourceMappingToFile(".paramsReady", ".paramsReady")
}

// MarkConfigReady indicates that all configs (including user configs) are in
// place, and the mandelbox is ready to start the protocol + application.
func (mandelbox *mandelboxData) MarkConfigReady() error {
	return mandelbox.writeResourceMappingToFile(".configReady", ".configReady")
}

func (mandelbox *mandelboxData) getResourceMappingDir() string {
	return utils.Sprintf("%s%s/mandelboxResourceMappings/", utils.WhistDir, mandelbox.GetID())
}

func (mandelbox *mandelboxData) createResourceMappingDir() error {
	err := os.MkdirAll(mandelbox.getResourceMappingDir(), 0777)
	if err != nil {
		return utils.MakeError("Failed to create dir %s. Error: %s", mandelbox.getResourceMappingDir(), err)
	}
	return nil
}

func (mandelbox *mandelboxData) cleanResourceMappingDir() {
	err := os.RemoveAll(mandelbox.getResourceMappingDir())
	if err != nil {
		logger.Errorf("Failed to remove dir %s. Error: %s", mandelbox.getResourceMappingDir(), err)
	}
}

func (mandelbox *mandelboxData) writeResourceMappingToFile(filename, data string) (err error) {
	file, err := os.OpenFile(mandelbox.getResourceMappingDir()+filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0777)
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
