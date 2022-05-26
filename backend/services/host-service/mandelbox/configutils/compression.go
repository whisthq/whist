package configutils // import "github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"

import (
	"archive/tar"
	"bytes"
	"io"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"strings"
	"compress/gzip"
	"encoding/base64"

	"github.com/pierrec/lz4/v4"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// ExtractTarLz4 extracts a tar.lz4 file in memory to the specified directory.
// Returns the total number of bytes extracted or an error.
func ExtractTarLz4(file []byte, dir string) (int64, error) {
	// Create the destination directory if it doesn't exist.
	_, err := os.Stat(dir)
	if err != nil {
		if os.IsNotExist(err) {
			if err := os.MkdirAll(dir, 0755); err != nil {
				return 0, utils.MakeError("failed to create directory %s: %v", dir, err)
			}
		} else {
			return 0, utils.MakeError("failed to stat directory %s: %v", dir, err)
		}
	}

	// Track the total number of bytes extracted
	var totalBytes int64

	// Extracting the tar.lz4 file involves:
	// 1. Decompress the the lz4 file into a tar
	// 2. Untar the config to the target directory
	dataReader := bytes.NewReader(file)
	lz4Reader := lz4.NewReader(dataReader)
	tarReader := tar.NewReader(lz4Reader)

	for {
		// Read the header for the next file in the tar
		header, err := tarReader.Next()

		// If we reach EOF, that means we are done untaring
		if err != nil {
			if err == io.EOF {
				break
			}
			return totalBytes, utils.MakeError("error reading tar header: %v", err)
		}

		// This really should not happen
		if header == nil {
			break
		}

		path := filepath.Join(dir, header.Name)

		// Check that the path is within the destination directory
		// to avoid the zip slip vulnerability
		// See: https://snyk.io/research/zip-slip-vulnerability
		if !strings.HasPrefix(path, dir) {
			return totalBytes, utils.MakeError("destination path %s is illegal", path)
		}

		info := header.FileInfo()

		// Create directory if it doesn't exist, otherwise go next
		if info.IsDir() {
			if err = os.MkdirAll(path, info.Mode()); err != nil {
				return totalBytes, utils.MakeError("error creating directory: %v", err)
			}
			continue
		}

		// Create file and copy contents
		file, err := os.OpenFile(path, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, info.Mode())
		if err != nil {
			return totalBytes, utils.MakeError("error opening file: %v", err)
		}

		numBytes, err := io.Copy(file, tarReader)
		if err != nil {
			file.Close()
			return totalBytes, utils.MakeError("error copying data to file: %v", err)
		}

		totalBytes += numBytes

		// Manually close file instead of defer otherwise files are only
		// closed when ALL files are done unpacking
		if err := file.Close(); err != nil {
			logger.Warningf("Failed to close file %s: %v", path, err)
		}
	}

	return totalBytes, nil
}

// CompressTarLz4 takes a directory and compresses it into a tar.lz4 file in memory.
func CompressTarLz4(dir string) ([]byte, error) {
	// Check that the directory exists and is a directory
	info, err := os.Stat(dir)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, utils.MakeError("directory %s does not exist", dir)
		}
		return nil, utils.MakeError("failed to stat directory %s: %v", dir, err)
	}
	if !info.IsDir() {
		return nil, utils.MakeError("%s is not a directory", dir)
	}

	// Create a buffer to write the tar.lz4 file to
	buf := new(bytes.Buffer)

	// Compressing the tar.lz4 file involves:
	// 1. Create a tar of the directory
	// 2. Compress the tar into a lz4 file
	lz4Writer := lz4.NewWriter(buf)
	tarWriter := tar.NewWriter(lz4Writer)

	// Walk the directory and add all files to the tar
	err = filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return utils.MakeError("error walking directory: %v", err)
		}

		// Create header for the file
		header, err := tar.FileInfoHeader(info, info.Name())
		if err != nil {
			return utils.MakeError("error creating tar header for file %s: %v", path, err)
		}

		// Convert the path to a relative path from the base directory.
		// This is required because we need the header name to be the
		// full relative path from the base directory, info.Name()
		// only gives the base name of the file.
		// See: https://pkg.go.dev/archive/tar#FileInfoHeader
		relPath, err := filepath.Rel(dir, path)
		if err != nil {
			return utils.MakeError("error converting path %s to relative path: %v", path, err)
		}
		header.Name = relPath

		// Write the header to the tar
		if err := tarWriter.WriteHeader(header); err != nil {
			return utils.MakeError("error writing tar header for file %s: %v", path, err)
		}

		// Skip directories and symlinks since there is no data to tar
		// source: https://medium.com/@komuw/just-like-you-did-fbdd7df829d3
		if !info.Mode().IsRegular() {
			return nil
		}

		// Open the file and copy its contents to the tar
		file, err := os.Open(path)
		if err != nil {
			return utils.MakeError("error opening file %s: %v", path, err)
		}

		if _, err := io.Copy(tarWriter, file); err != nil {
			file.Close()
			return utils.MakeError("error copying data from file %s: %v", path, err)
		}

		// Manually close file instead of defer otherwise files are only
		// closed when ALL files are done packing
		if err := file.Close(); err != nil {
			logger.Warningf("Failed to close file %s: %v", path, err)
		}

		return nil
	})
	if err != nil {
		return nil, utils.MakeError("error tarring directory: %v", err)
	}

	// Close writers manually instead of defer to ensure that all
	// contents are written to buffer before returning.
	tarWriter.Close()
	lz4Writer.Close()

	return buf.Bytes(), nil
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
			if string(testFileContents) != matchingFileBuf.String() {
				return utils.MakeError("file contents don't match for file %s: '%s' vs '%s'", filePath, testFileContents, matchingFileBuf.Bytes())
			}
		}

		return nil
	})
}

// Extract gzip-compressed binary data (acknowledged to https://gist.github.com/xyproto/f4915d7e208771f3adc4)
func inflateGzip(w io.Writer, data []byte) error {
	gr, err := gzip.NewReader(bytes.NewBuffer(data))
	defer gr.Close()
	
	data, err = ioutil.ReadAll(gr)
	if err != nil {
		return utils.MakeError("Couldn't extract inflated string from gzip reader: %s", err)
	}
	
	w.Write(data)
	
	return nil
}
// Inflate a gzip-compressed string (encoded according to base-64 protocol)
func gzipInflateString(compressedGzipString string) (string, error) {
	base64DecodedData, err := base64.StdEncoding.DecodeString(compressedGzipString)
	if err != nil {
		return utils.MakeError("Couldn't decode received string: %s", err)
	}
	var gzipDecodedDataBuffer bytes.Buffer
	err = inflateGzip(&gzipDecodedDataBuffer, base64DecodedData)
	if err != nil {
		return utils.MakeError("Couldn't inflate decoded string: %s", err)
	}
	return gzipDecodedDataBuffer.String()
}