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
	UserInitialBrowserDirInMbox string = utils.WhistDir + "userConfigs/"
	UserInitialBrowserFile      string = "user-initial-file"
)

// BrowserData is a collection of possible browser datas a user generates
type BrowserData struct {
	// CookieJSON is the user's cookie sqlite3 file in a json string format
	CookiesJSON types.Cookies `json:"cookiesJSON,omitempty"`
	// Bookmarks is the user's bookmark json file
	Bookmarks *configutils.Bookmarks `json:"bookmarks,omitempty"`
	// Extensions is a comma spliced string that represents the users browser extensions
	Extensions types.Extensions `json:"extensions,omitempty"`
	// Preferences is the user's preferences json file
	Preferences types.Preferences `json:"preferences,omitempty"`
}

// WriteUserInitialBrowserData writes the user's initial browser data received
// through JSON transport on top of the user configs that have already been
// loaded.
func (mandelbox *mandelboxData) WriteUserInitialBrowserData(initialBrowserData BrowserData) error {
	destDir := path.Join(mandelbox.GetUserConfigDir(), UnpackedConfigsDirectoryName)

	// Create destination directory if not exists
	if _, err := os.Stat(destDir); os.IsNotExist(err) {
		if err := os.MkdirAll(destDir, 0777); err != nil {
			return utils.MakeError("Could not make dir %s. Error: %s", destDir, err)
		}

		defer func() {
			cmd := exec.Command("chown", "-R", "ubuntu", destDir)
			cmd.Run()
			cmd = exec.Command("chmod", "-R", "777", destDir)
			cmd.Run()
		}()
	}

	// If bookmarks is empty, we will not write it
	if initialBrowserData.Bookmarks == nil || len(initialBrowserData.Bookmarks.Roots) == 0 {
		initialBrowserData.Bookmarks = nil
	}

	// Convert struct into JSON string
	data, err := json.Marshal(initialBrowserData)

	if err != nil {
		return utils.MakeError("Could not marshal initialBrowserData: %v", initialBrowserData)
	}

	filePath := path.Join(destDir, UserInitialBrowserFile)

	// Save the browser data into a file
	if err := utils.WriteToNewFile(filePath, string(data)); err != nil {
		return utils.MakeError("could not create %s file. Error: %v", filePath, err)
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
