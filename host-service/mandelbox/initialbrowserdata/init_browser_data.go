package initialbrowserdata

import (
	"os"
	"os/exec"
	"path"

	"github.com/fractal/fractal/host-service/utils"
	mandelboxData "github.com/fractal/fractal/host-service/mandelbox"
	logger "github.com/fractal/fractal/host-service/whistlogger"
)

type BrowserData struct {
	// CookieJSON is the user's cookie sqlite3 file in a string format
	CookiesJSON 	string
	// BookmarkJSON is the user's bookmark json file
	BookmarksJSON 	string
}

// WriteUserInitialBrowserData writes the user's initial browser data to file(s)
// received through JSON transport for later use in the mandelbox
func (mandelbox *mandelboxData) WriteUserInitialBrowserData(initialBrowserData BrowserData) error {

	cookieJSON := initialBrowserData.CookiesJSON
	bookmarksJSON := initialBrowserData.BookmarksJSON

	// Avoid doing work for empty string/array string
	if len(cookieJSON) <= 2 && len(bookmarksJSON) <= 2 {
		logger.Infof("Not writing to file as user initial browser data is empty.")
		// the browser data can be empty
		return nil
	}

	// The initial browser cookies will use the same directory as the user config
	// Make directories for user configs if not exists
	configDir := mandelbox.getUserConfigDir()
	if _, err := os.Stat(configDir); os.IsNotExist(err) {
		if err := os.MkdirAll(configDir, 0777); err != nil {
			return utils.MakeError("Could not make dir %s. Error: %s", configDir, err)
		}
		defer func() {
			cmd := exec.Command("chown", "-R", "ubuntu", configDir)
			cmd.Run()
			cmd = exec.Command("chmod", "-R", "777", configDir)
			cmd.Run()
		}()
	}

	unpackedConfigDir := path.Join(configDir, mandelbox.getUnpackedConfigsDirectoryName())
	if _, err := os.Stat(unpackedConfigDir); os.IsNotExist(err) {
		if err := os.MkdirAll(unpackedConfigDir, 0777); err != nil {
			return utils.MakeError("Could not make dir %s. Error: %s", unpackedConfigDir, err)
		}
	}

	// Begin writing user initial browser data
	cookieFilePath := path.Join(unpackedConfigDir, utils.UserInitialCookiesFile)

	if err := utils.WriteToNewFile(cookieFilePath, cookieJSON); err != nil {
		return utils.MakeError("error creating cookies file. Error: %v", err)
	}

	bookmarkFilePath := path.Join(unpackedConfigDir, utils.UserInitialBookmarksFile)

	if err := utils.WriteToNewFile(bookmarkFilePath, bookmarksJSON); err != nil {
		return utils.MakeError("error creating bookmarks file. Error: %v", err)
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
