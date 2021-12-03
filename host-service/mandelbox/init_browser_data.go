package mandelbox

import (
	"os"
	"os/exec"
	"path"

	"github.com/fractal/fractal/host-service/utils"
	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	logger "github.com/fractal/fractal/host-service/whistlogger"
)

// This contains the path and file names related to browser data
const (
	UserInitialBrowserDir    string = utils.WhistDir + "userConfigs/"
	UserInitialCookiesFile   string = "user-initial-cookies"
	UserInitialBookmarksFile string = "user-initial-bookmarks"
)

// WriteUserInitialBrowserData writes the user's initial browser data to file(s)
// received through JSON transport for later use
func WriteUserInitialBrowserData(initialBrowserData BrowserData, destDir string) error {

	cookieJSON := initialBrowserData.CookiesJSON
	bookmarksJSON := initialBrowserData.BookmarksJSON

	// Avoid doing work for empty string/array string
	if len(cookieJSON) <= 2 && len(bookmarksJSON) <= 2 {
		logger.Infof("Not writing to file as user initial browser data is empty.")
		// the browser data can be empty
		return nil
	}

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

	// Begin writing user initial browser data
	cookieFilePath := path.Join(destDir, UserInitialCookiesFile)

	if err := utils.WriteToNewFile(cookieFilePath, cookieJSON); err != nil {
		return utils.MakeError("error creating cookies file. Error: %v", err)
	}

	bookmarkFilePath := path.Join(destDir, UserInitialBookmarksFile)

	if err := utils.WriteToNewFile(bookmarkFilePath, bookmarksJSON); err != nil {
		return utils.MakeError("error creating bookmarks file. Error: %v", err)
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
