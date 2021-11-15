package configutils

import (
	"bytes"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"testing"

	"github.com/fractal/fractal/host-service/utils"
)

// TestCompressionIntegration compresses and decompresses a test directory
// and compares the result with the original directory.
func TestCompressionIntegration(t *testing.T) {
	sourceDir := t.TempDir()
	if err := SetupTestDir(sourceDir); err != nil {
		t.Fatalf("failed to setup test directory: %v", err)
	}

	compressedDir, err := CompressTarLz4(sourceDir)
	if err != nil {
		t.Fatalf("failed to compress directory: %v", err)
	}

	destinationDir := t.TempDir()
	if err := ExtractTarLz4(compressedDir, destinationDir); err != nil {
		t.Fatalf("failed to decompress directory: %v", err)
	}

	if err := ValidateDirectoryContents(sourceDir, destinationDir); err != nil {
		t.Fatalf("failed to validate directory contents: %v", err)
	}
}

// SetupTestDir fills the specified directory with some test files.
func SetupTestDir(testDir string) error {
	// Write some directories with text files
	for i := 0; i < 300; i++ {
		tempDir := path.Join(testDir, utils.Sprintf("dir%d", i))
		if err := os.Mkdir(tempDir, 0777); err != nil {
			return utils.MakeError("failed to create temp dir %s: %v", tempDir, err)
		}

		for j := 0; j < 3; j++ {
			filePath := path.Join(tempDir, utils.Sprintf("file-%d.txt", j))
			fileContents := utils.Sprintf("This is file %d in directory %d.", j, i)
			err := os.WriteFile(filePath, []byte(fileContents), 0777)
			if err != nil {
				return utils.MakeError("failed to write to file %s: %v", filePath, err)
			}
		}
	}

	// Write a nested directory with files inside
	nestedDir := path.Join(testDir, "dir10", "nested")
	if err := os.Mkdir(nestedDir, 0777); err != nil {
		return utils.MakeError("failed to create nested dir %s: %v", nestedDir, err)
	}
	nestedFile := path.Join(nestedDir, "nested-file.txt")
	fileContents := utils.Sprintf("This is a nested file.")
	err := os.WriteFile(nestedFile, []byte(fileContents), 0777)
	if err != nil {
		return utils.MakeError("failed to write to file %s: %v", nestedFile, err)
	}

	// Write some files not in a directory
	for i := 0; i < 5; i++ {
		filePath := path.Join(testDir, utils.Sprintf("file-%d.txt", i))
		fileContents := utils.Sprintf("This is file %d in directory %s.", i, "none")
		err := os.WriteFile(filePath, []byte(fileContents), 0777)
		if err != nil {
			return utils.MakeError("failed to write to file %s: %v", filePath, err)
		}
	}

	return nil
}

// ValidateDirectoryContents checks if all directories and files in the
// old directory are present in the new directory and have the same contents.
func ValidateDirectoryContents(oldDir, newDir string) error {
	return filepath.Walk(oldDir, func(filePath string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		relativePath, err := filepath.Rel(oldDir, filePath)
		if err != nil {
			return utils.MakeError("error getting relative path for %s: %v", filePath, err)
		}

		newFilePath := filepath.Join(newDir, relativePath)
		matchingFile, err := os.Open(newFilePath)
		if err != nil {
			return utils.MakeError("error opening matching file %s: %v", newFilePath, err)
		}
		defer matchingFile.Close()

		matchingFileInfo, err := matchingFile.Stat()
		if err != nil {
			return utils.MakeError("error reading stat for file: %s: %v", newFilePath, err)
		}

		// If one is a directory, both should be
		if info.IsDir() {
			if !matchingFileInfo.IsDir() {
				return utils.MakeError("expected %s to be a directory", newFilePath)
			}
		} else {
			testFileContents, err := ioutil.ReadFile(filePath)
			if err != nil {
				return utils.MakeError("error reading test file %s: %v", filePath, err)
			}

			matchingFileBuf := bytes.NewBuffer(nil)
			_, err = matchingFileBuf.ReadFrom(matchingFile)
			if err != nil {
				return utils.MakeError("error reading matching file %s: %v", newFilePath, err)
			}

			// Check contents match
			if string(testFileContents) != string(matchingFileBuf.Bytes()) {
				return utils.MakeError("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContents, matchingFileBuf.Bytes())
			}
		}

		return nil
	})
}
