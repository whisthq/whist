package mandelbox

import (
	"encoding/json"
	"os"
	"os/exec"
	"path"

	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// This contains the path and file names related to browser data
var (
	UserInitialBrowserDirInMbox string = path.Join(utils.WhistDir, "userConfigs/")
	UserInitialBrowserFile      string = "user-initial-file"
)

// WriteUserInitialBrowserData writes the user's initial browser data received
// through JSON transport on top of the user configs that have already been
// loaded.
func (mandelbox *mandelboxData) WriteUserInitialBrowserData(initialBrowserData types.BrowserData) error {
	destDir := path.Join(mandelbox.GetUserConfigDir(), UnpackedConfigsDirectoryName)

	// Create destination directory if not exists
	if _, err := os.Stat(destDir); os.IsNotExist(err) {
		if err := os.MkdirAll(destDir, 0777); err != nil {
			return utils.MakeError("could not make dir %s: %s", destDir, err)
		}

		defer func() {
			cmd := exec.Command("chown", "-R", "ubuntu", destDir)
			cmd.Run()
			cmd = exec.Command("chmod", "-R", "777", destDir)
			cmd.Run()
		}()
	}

	// Inflate gzip-compressed user browser data
	inflatedBrowserData, err := configutils.GzipInflateString(string(initialBrowserData))
	if err != nil {
		return utils.MakeError("error inflating user browser data: %s", err)
	}

	// Verify that the string is a valid JSON
	_, err = json.Marshal(inflatedBrowserData)
	if err != nil {
		return utils.MakeError("invalid JSON string received as browser data: %s", err)
	}

	filePath := path.Join(destDir, UserInitialBrowserFile)
	// Save the browser data into a file
	if err := utils.WriteToNewFile(filePath, inflatedBrowserData); err != nil {
		return utils.MakeError("could not create %s file: %s", filePath, err)
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
