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

// BrowserData is a collection of all possible browser data items a user can generate
type BrowserData struct {
	// CookieJSON is the user's cookie sqlite3 file in a json string format
	CookiesJSON types.Cookies `json:"cookiesJSON,omitempty"`
	// Bookmarks is the user's bookmark json file
	Bookmarks *configutils.Bookmarks `json:"bookmarks,omitempty"`
	// Extensions is a comma spliced string that represents the users browser extensions
	Extensions types.Extensions `json:"extensions,omitempty"`
	// Preferences is the user's preferences json file
	Preferences types.Preferences `json:"preferences,omitempty"`
	// LocalStorage is the user's local storage files as a JSON string
	LocalStorage types.LocalStorage `json:"local_storage,omitempty"`
	// ExtensionSettings is the user's extension settings as a JSON string
	ExtensionSettings types.ExtensionSettings `json:"extension_settings,omitempty"`
	// ExtensionState is the user's extension state as a JSON string
	ExtensionState types.ExtensionState `json:"extension_state,omitempty"`
}

// Custom-defined UnmarshalJSON function to handle the empty-string case correctly.
func (browserdata *BrowserData) UnmarshalJSON(data []byte) error {
	if string(data) == `""` || string(data) == `''` || string(data) == "" {
		return nil
	}
	type tmp BrowserData
	return json.Unmarshal(data, (*tmp)(browserdata))
}

// UnmarshalBookmarks takes a JSON string containing all the user browser data
// and unmarshals it into a BrowserData struct, returning the struct
// and any errors encountered.
func UnmarshalBrowserData(browser_data types.BrowserData) (BrowserData, error) {
	var browserDataObj BrowserData
	err := json.Unmarshal([]byte(browser_data), &browserDataObj)
	return browserDataObj, err
}

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

	// Unmarshal bookmarks into proper format
	var browserData BrowserData
	if len(inflatedBrowserData) > 0 {
		browserData, err = UnmarshalBrowserData(types.BrowserData(inflatedBrowserData))
		if err != nil {
			// BrowserData import errors are not fatal
			logger.Errorf("error unmarshalling user browser data for mandelbox %s: %s", mandelbox.GetID(), err)
		}
	}

	// If bookmarks is empty, we will not write it
	if browserData.Bookmarks == nil || len(browserData.Bookmarks.Roots) == 0 {
		browserData.Bookmarks = nil
	}

	// Convert struct into JSON string
	logger.Infof("Got browser data", browserData)

	data, err := json.Marshal(browserData)

	if err != nil {
		return utils.MakeError("could not marshal browserData: %v", browserData)
	}

	filePath := path.Join(destDir, UserInitialBrowserFile)

	// Save the browser data into a file
	if err := utils.WriteToNewFile(filePath, string(data)); err != nil {
		return utils.MakeError("could not create %s file: %s", filePath, err)
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
