package mandelbox

import (
	"os"
	"os/exec"
	"path"

	"github.com/whisthq/whist/core-go/types"
	"github.com/whisthq/whist/core-go/utils"
	logger "github.com/whisthq/whist/core-go/whistlogger"
)

// This contains the path and file names related to browser data
const (
	UserInitialBrowserDir    	string = utils.WhistDir + "userConfigs/"
	UserInitialCookiesFile   	string = "user-initial-cookies"
	UserInitialBookmarksFile 	string = "user-initial-bookmarks"
	UserInitialExtensionsFile 	string = "user-initial-extensions"
)

// BrowserData is a collection of possible browser datas a user generates
type BrowserData struct {
	// CookieJSON is the user's cookie sqlite3 file in a string format
	CookiesJSON types.Cookies
	// BookmarkJSON is the user's bookmark json file
	BookmarksJSON types.Bookmarks
	// Extensions is a comma spliced string that represents the users browser extensions
	Extensions types.Extensions
}

// WriteUserInitialBrowserData writes the user's initial browser data to file(s)
// received through JSON transport for later use
func WriteUserInitialBrowserData(initialBrowserData BrowserData, destDir string) error {
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
	extensionFilePath := path.Join(destDir, UserInitialExtensionsFile)

	browserDataInfos := [][]string{
		{string(initialBrowserData.CookiesJSON), cookieFilePath, "cookies"},
		{string(initialBrowserData.BookmarksJSON), bookmarkFilePath, "bookmarks"},
		{string(initialBrowserData.Extensions), extensionFilePath, "extensions"},
	}

	for _, browserDataInfo := range browserDataInfos {
		content := browserDataInfo[0]
		filePath := browserDataInfo[1]
		contentType := browserDataInfo[2]

		if len(content) < 0 {
			logger.Infof("Did not create new file: %s of type %v as content was empty", filePath, contentType)
			continue
		}
		
		if err := utils.WriteToNewFile(filePath, content); err != nil {
			logger.Errorf("could not create %s file. Error: %v", contentType, err)
		}
	}

	logger.Infof("Finished storing user initial browser data.")

	return nil
}
