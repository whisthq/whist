package configutils

import (
	"testing"
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
	if _, err := ExtractTarLz4(compressedDir, destinationDir); err != nil {
		t.Fatalf("failed to decompress directory: %v", err)
	}

	if err := ValidateDirectoryContents(sourceDir, destinationDir); err != nil {
		t.Fatalf("failed to validate directory contents: %v", err)
	}
}
