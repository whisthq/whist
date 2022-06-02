package configutils

import (
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"testing"
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
	tarFile, err := ioutil.ReadFile(tarOutputFile)
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
	err = os.WriteFile(tarOutputFile, compressedDir, 0777)
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

// TestGzipCompression compresses and decompresses a string using the Gzip
// protocol and compares the result with the original string.
func TestGzipCompression(t *testing.T) {
	// 1. Test compression and decompression of a non-empty string
	sampleJsonData := `{"dark_mode":false,"desired_timezone":"America/New_York","client_dpi":192,"restore_last_session":true,"kiosk_mode":false,"initial_key_repeat":300,"key_repeat":30,"local_client":true,"user_agent":"Mozilla/5.0 (Macintosh; Intel Mac OS X 12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36","longitude":"103.851959","latitude":"1.290270","system_languages":"en_US","browser_languages":"en-US,en","user_locale":{"LC_COLLATE":"en_US.UTF-8","LC_CTYPE":"en_US.UTF-8","LC_MESSAGES":"en_US.UTF-8","LC_MONETARY":"en_US.UTF-8","LC_NUMERIC":"en_US.UTF-8","LC_TIME":"en_US.UTF-8"},"client_os":"darwin"}`
	deflatedJSONData, err := GzipDeflateString(string(sampleJsonData))
	if err != nil {
		t.Fatalf("could not deflate string with Gzip: %v", err)
	}
	if len(deflatedJSONData) >= len(sampleJsonData) {
		t.Fatalf("Gzip compression failed to reduce string length")
	}
	inflatedJSONData, err := GzipInflateString(deflatedJSONData)
	if err != nil {
		t.Fatalf("Couldn't inflate string with Gzip: %s", err)
	}
	if inflatedJSONData != sampleJsonData {
		t.Fatalf("Inflating Gzip-deflated string did not yield original string: %s", err)
	}

	// 2. Test compression and decompression of a empty string
	emptyString := ``
	deflatedEmptyString, err := GzipDeflateString(string(emptyString))
	if err != nil {
		t.Fatalf("could not deflate empty string with Gzip: %v", err)
	}
	if len(deflatedEmptyString) != 0 {
		t.Fatalf("Gzip compression failed when called on empty string")
	}
	inflatedEmptyString, err := GzipInflateString(deflatedEmptyString)
	if err != nil {
		t.Fatalf("Couldn't inflate empty string with Gzip: %s", err)
	}
	if len(inflatedEmptyString) != 0 || inflatedEmptyString != emptyString {
		t.Fatalf("Inflating Gzip-deflated empty string did not yield an empty string: %s", err)
	}
}
