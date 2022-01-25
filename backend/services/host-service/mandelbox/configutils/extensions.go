package configutils // import "github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"

import (
	"encoding/json"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	types "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

const ImportedExtensionFileName = "imported-extensions.json"

type ImportedExtensions struct {
	Extensions []string `json:"extensions"`
}

// GetImportedExtensions searches a given unpacked config directory looking
// for extensions that were previously imported and returns their IDs
// as a list of strings.
func GetImportedExtensions(dir string) []string {
	// Read the extensions file from the config directory
	extensionsFilePath := filepath.Join(dir, ImportedExtensionFileName)
	extensionsFile, err := os.Open(extensionsFilePath)
	if err != nil {
		logger.Infof("failed to open extensions file %s: %v", extensionsFilePath, err)
		return []string{}
	}
	defer extensionsFile.Close()

	// Unmarshal the extensions file into a list of strings
	extensionsFileBytes, err := ioutil.ReadAll(extensionsFile)
	if err != nil {
		logger.Infof("failed to read extensions file %s: %v", extensionsFilePath, err)
		return []string{}
	}

	var extensions ImportedExtensions
	if err := json.Unmarshal(extensionsFileBytes, &extensions); err != nil {
		logger.Infof("failed to unmarshal extensions file %s: %v", extensionsFilePath, err)
		return []string{}
	}

	return extensions.Extensions
}

// UpdateImportedExtensions updates the list of imported extensions
// with the given list of extension IDs, adding those that are not
// already present.
func UpdateImportedExtensions(currentExtensions []string, newExtensions types.Extensions) []string {
	// Hash the current extensions for fast lookup
	currentExtensionsMap := make(map[string]bool)
	for _, extension := range currentExtensions {
		currentExtensionsMap[extension] = true
	}

	// Add any new extensions that are not already present
	splitExtensions := strings.Split(string(newExtensions), ",")
	var updatedExtensions []string
	for _, extension := range splitExtensions {
		extensionString := string(extension)
		if _, ok := currentExtensionsMap[extensionString]; !ok {
			updatedExtensions = append(updatedExtensions, extensionString)
		}
	}

	return append(currentExtensions, updatedExtensions...)
}

// SaveImportedExtensions writes a list of extension IDs to a file in the
// given config directory.
func SaveImportedExtensions(dir string, extensions []string) error {
	// Marshal the extensions into a JSON string
	extensionsBytes, err := json.Marshal(ImportedExtensions{Extensions: extensions})
	if err != nil {
		return utils.MakeError("failed to marshal extensions: %v", err)
	}

	// Write the extensions to a file in the config directory
	extensionsFilePath := filepath.Join(dir, ImportedExtensionFileName)
	if err := ioutil.WriteFile(extensionsFilePath, extensionsBytes, 0777); err != nil {
		return utils.MakeError("failed to write extensions file %s: %v", extensionsFilePath, err)
	}

	return nil
}
