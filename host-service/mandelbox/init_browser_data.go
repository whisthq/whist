package mandelbox

import (
	"os"
	"os/exec"
	"path"

	"github.com/fractal/whist/host-service/utils"
	logger "github.com/fractal/whist/host-service/whistlogger"
)

// This contains the path and file names related to browser data
const (
	UserInitialBrowserDir    string = utils.WhistDir + "userConfigs/"
	UserInitialCookiesFile   string = "user-initial-cookies"
	UserInitialBookmarksFile string = "user-initial-bookmarks"
)

// BrowserData is a collection of possible browser datas a user generates
type BrowserData struct {
	// CookieJSON is the user's cookie sqlite3 file in a string format
	CookiesJSON 	string
	// BookmarkJSON is the user's bookmark json file
	BookmarksJSON 	string
}

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
	bookmarkFilePath := path.Join(destDir, UserInitialBookmarksFile)

	browserDataFile := [][]string{
		{cookieJSON, cookieFilePath, "cookies"},
		{bookmarksJSON, bookmarkFilePath, "bookmarks"}
	}

	for _, browserDataFile := range s {
		content := browserDataFile[0]
		filePath := browserDataFile[1]
		contentType := browserDataFile[2]

		if len(browserDataFile[0]) > 0 {
			if err := utils.WriteToNewFile(browserDataFile[1], browserDataFile[0]); err != nil {
				logger.Errorf("could not create %s file. Error: %v", contentType, err)
			}
		}
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
