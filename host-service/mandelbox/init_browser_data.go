package mandelbox

import (
	"os"
	"os/exec"
	"path"

	"github.com/whisthq/whist/host-service/mandelbox/types"
	"github.com/whisthq/whist/host-service/utils"
	logger "github.com/whisthq/whist/host-service/whistlogger"
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
	CookiesJSON 	types.Cookies
	// BookmarkJSON is the user's bookmark json file
	BookmarksJSON 	types.Bookmarks
}

// WriteUserInitialBrowserData writes the user's initial browser data to file(s)
// received through JSON transport for later use
func WriteUserInitialBrowserData(initialBrowserData BrowserData, destDir string) error {
	// Avoid doing work for empty string/array string
	if len(initialBrowserData.CookiesJSON) <= 2 && len(initialBrowserData.BookmarksJSON) <= 2 {
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

	browserDataInfos := [][]string{
		{string(initialBrowserData.CookiesJSON), cookieFilePath, "cookies"},
		{string(initialBrowserData.BookmarksJSON), bookmarkFilePath, "bookmarks"},
	}

	for _, browserDataInfo := range  browserDataInfos {
		content := browserDataInfo[0]
		filePath := browserDataInfo[1]
		contentType := browserDataInfo[2]

		if len(content) > 0 {
			if err := utils.WriteToNewFile(filePath, content); err != nil {
				logger.Errorf("could not create %s file. Error: %v", contentType, err)
			}
		}
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
