package configutils

import (
	"os/exec"
	"path"
	"testing"

	"github.com/spf13/afero"
)

// TestExtraction creates a small tar.lz4 file using system's tar program
// and validates the extraction.
func TestExtraction(t *testing.T) {
	// Create a temp directory to hold test files.
	sourceDir := t.TempDir()
	if err := SetupTestDir(sourceDir); err != nil {
		t.Fatalf("failed to setup test directory: %v", err)
	}

	// Create a tar.lz4 file from the test directory.
	tarOutputDir := t.TempDir()
	tarOutputFile := path.Join(tarOutputDir, "/test.tar.lz4")
	tarCommand := exec.Command("/usr/bin/tar", "-I", "lz4", "-C", sourceDir, "-cf", tarOutputFile, ".")
	tarCommandOutput, err := tarCommand.CombinedOutput()
	if err != nil {
		t.Fatalf("error creating tar file: %v, output: %s", err, tarCommandOutput)
	}

	// Read the tar file into memory
	tarFile, err := afero.ReadFile(fs, tarOutputFile)
	if err != nil {
		t.Fatalf("error reading tar file: %v", err)
	}

	// Extract the tar.lz4 file using our own code
	destinationDir := t.TempDir()
	if _, err := ExtractTarLz4(tarFile, destinationDir); err != nil {
		t.Fatalf("error extracting tar file: %v", err)
	}

	// Validate the contents of the extracted directory
	if err := ValidateDirectoryContents(sourceDir, destinationDir); err != nil {
		t.Fatalf("error validating directory contents: %v", err)
	}
}

// TestCompression creates a small tar.lz4 file using our own code and validates
// that it can be successfully extracted using system's tar program.
func TestCompression(t *testing.T) {
	sourceDir := t.TempDir()
	if err := SetupTestDir(sourceDir); err != nil {
		t.Fatalf("failed to setup test directory: %v", err)
	}

	compressedDir, err := CompressTarLz4(sourceDir)
	if err != nil {
		t.Fatalf("failed to compress directory: %v", err)
	}

	// Write the compressed lz4 to a file
	tarOutputDir := t.TempDir()
	tarOutputFile := path.Join(tarOutputDir, "/test.tar.lz4")
	err = afero.WriteFile(fs, tarOutputFile, compressedDir, 0777)
	if err != nil {
		t.Fatalf("error writing tar file: %v", err)
	}

	// Extract the tar.lz4 file using system's tar program
	destinationDir := t.TempDir()
	tarCommand := exec.Command("/usr/bin/tar", "-I", "lz4", "-xf", tarOutputFile, "-C", destinationDir)
	tarCommandOutput, err := tarCommand.CombinedOutput()
	if err != nil {
		t.Fatalf("error extracting tar file: %v, output: %s", err, tarCommandOutput)
	}

	if err := ValidateDirectoryContents(sourceDir, destinationDir); err != nil {
		t.Fatalf("error validating directory contents: %v", err)
	}
}

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
	if _, err := ExtractTarLz4(compressedDir, destinationDir); err != nil {
		t.Fatalf("failed to decompress directory: %v", err)
	}

	if err := ValidateDirectoryContents(sourceDir, destinationDir); err != nil {
		t.Fatalf("failed to validate directory contents: %v", err)
	}
}
