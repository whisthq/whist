package configutils

import (
	"path/filepath"
	"testing"

	"github.com/spf13/afero"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// TestGetImportedExtensions tests the helper function that finds
// previously imported extensions.
func TestGetImportedExtensions(t *testing.T) {
	testDir := t.TempDir()

	// Test that an empty list is returned when no extensions are imported.
	t.Run("no imported-extensions.json file", func(t *testing.T) {
		extensions := GetImportedExtensions(testDir)
		if len(extensions) != 0 {
			t.Errorf("expected empty list, got %v", extensions)
		}
	})

	// Test that an empty list is returned when the imported-extensions.json file is invalid.
	t.Run("invalid imported-extensions.json file", func(t *testing.T) {
		extensionsFilePath := filepath.Join(testDir, ImportedExtensionFileName)
		extensionsFile, err := fs.Create(extensionsFilePath)
		if err != nil {
			t.Fatalf("failed to create extensions file %s: %v", extensionsFilePath, err)
		}
		defer extensionsFile.Close()

		extensionsFileBytes := []byte("invalid json")
		if _, err := extensionsFile.Write(extensionsFileBytes); err != nil {
			t.Fatalf("failed to write extensions file %s: %v", extensionsFilePath, err)
		}

		extensions := GetImportedExtensions(testDir)
		if len(extensions) != 0 {
			t.Errorf("expected empty list, got %v", extensions)
		}
	})

	// Test that the list of imported extensions is returned when the
	// imported-extensions.json file is valid.
	t.Run("valid imported-extensions.json file", func(t *testing.T) {
		extensionsFilePath := filepath.Join(testDir, ImportedExtensionFileName)
		extensionsFile, err := fs.Create(extensionsFilePath)
		if err != nil {
			t.Fatalf("failed to create extensions file %s: %v", extensionsFilePath, err)
		}
		defer extensionsFile.Close()

		extensionsFileBytes := []byte("{\"extensions\":[\"extension1\",\"extension2\"]}")
		if _, err := extensionsFile.Write(extensionsFileBytes); err != nil {
			t.Fatalf("failed to write extensions file %s: %v", extensionsFilePath, err)
		}

		extensions := GetImportedExtensions(testDir)
		if len(extensions) != 2 {
			t.Errorf("expected 2 extensions, got %v", extensions)
		}

		expectedExtensions := []string{"extension1", "extension2"}
		for _, extension := range expectedExtensions {
			if !utils.StringSliceContains(extensions, extension) {
				t.Errorf("expected extension %s not found", extension)
			}
		}
	})
}

// TestUpdateImportedExtensions tests the helper function that updates
// the imported extensions list.
func TestUpdateImportedExtensions(t *testing.T) {
	baseExtensions := []string{"extension1", "extension2"}

	t.Run("overlapping extensions", func(t *testing.T) {
		newExtensions := types.Extensions("extension1,extension2,extension3")
		updatedExtensions := UpdateImportedExtensions(baseExtensions, newExtensions)

		if len(updatedExtensions) != 3 {
			t.Errorf("expected 3 extensions, got %v", updatedExtensions)
		}

		expectedExtensions := []string{"extension1", "extension2", "extension3"}
		for _, extension := range expectedExtensions {
			if !utils.StringSliceContains(updatedExtensions, extension) {
				t.Errorf("expected extension %s not found", extension)
			}
		}
	})

	t.Run("non-overlapping extensions", func(t *testing.T) {
		newExtensions := types.Extensions("extension3,extension4")
		updatedExtensions := UpdateImportedExtensions(baseExtensions, newExtensions)

		if len(updatedExtensions) != 4 {
			t.Errorf("expected 4 extensions, got %v", updatedExtensions)
		}

		expectedExtensions := []string{"extension1", "extension2", "extension3", "extension4"}
		for _, extension := range expectedExtensions {
			if !utils.StringSliceContains(updatedExtensions, extension) {
				t.Errorf("expected extension %s not found", extension)
			}
		}
	})
}

// TestSaveImportedExtensions tests the helper function that saves
// the imported extensions list.
func TestSaveImportedExtensions(t *testing.T) {
	testDir := t.TempDir()
	extensions := []string{"extension1", "extension2", "extension3"}

	err := SaveImportedExtensions(testDir, extensions)
	if err != nil {
		t.Fatalf("failed to save extensions: %v", err)
	}

	extensionsFilePath := filepath.Join(testDir, ImportedExtensionFileName)
	extensionsFile, err := fs.Open(extensionsFilePath)
	if err != nil {
		t.Fatalf("failed to open extensions file %s: %v", extensionsFilePath, err)
	}
	defer extensionsFile.Close()

	extensionsFileContents, err := afero.ReadAll(extensionsFile)
	if err != nil {
		t.Fatalf("failed to read extensions file %s: %v", extensionsFilePath, err)
	}

	expectedExtensionsFileContents := "{\"extensions\":[\"extension1\",\"extension2\",\"extension3\"]}"
	if string(extensionsFileContents) != expectedExtensionsFileContents {
		t.Errorf("expected saved extensions file to be %s, got %s", expectedExtensionsFileContents, extensionsFileContents)
	}
}
