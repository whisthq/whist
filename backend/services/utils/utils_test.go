package utils

import (
	"math/rand"
	"path"
	"sync"
	"testing"
	"time"

	"github.com/spf13/afero"
)

func TestPlaceholderUUIDs(t *testing.T) {
	// Test the placeholder UUID functions, only verify that they correspond
	// to the predefined warmup and test UUIDs.
	testMap := []struct {
		testName  string
		want, got string
	}{
		{"Warmup UUID", "11111111-1111-1111-1111-111111111111", PlaceholderWarmupUUID().String()},
		{"Test UUID", "22222222-2222-2222-2222-222222222222", PlaceholderTestUUID().String()},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request testName %s to be %v, got %v", value.testName, value.got, value.want)
		}
	}
}

func TestWaitWithDebugPrints(t *testing.T) {
	// Use a waitgroup and random goroutines to test WaitWithDebugPrints
	wg := sync.WaitGroup{}
	timeout := 1 * time.Second
	level := 2

	// Use goroutines with random duration
	for i := 1; i <= 5; i++ {
		wg.Add(1)

		go func() {
			defer wg.Done()
			r := rand.Intn(5)
			time.Sleep(time.Duration(r) * time.Second)
		}()
	}
	WaitWithDebugPrints(&wg, timeout, level)

	// Check if the wait group finished successfully
	wg.Wait()
}

func TestWaitForFileCreation(t *testing.T) {
	testDir := path.Join(TempDir, "testBase")

	// Setup the test directory to write the test file
	err := setupTestDirs(testDir)
	defer cleanupTestDirs()
	if err != nil {
		t.Fatalf("Failed to setup test directory: %v", err)
	}

	// Make a goroutine that waits for the test file to be created
	waitErrorChan := make(chan error)
	go func() {
		t.Logf("Testing wait for file creation on path: %v", testDir)

		err = WaitForFileCreation(testDir, "test-file.txt", 10*time.Second, nil)
		waitErrorChan <- err
	}()

	// Make another goroutine that writes the test file
	writeErrorChan := make(chan error)
	go func() {
		// Sleep to give some time to the file watcher to start
		time.Sleep(5 * time.Second)
		err := writeTestFile(testDir)
		writeErrorChan <- err
	}()

	// Check if WaitForFileCreation finished without any errors
	err = <-waitErrorChan
	if err != nil {
		t.Errorf("Error waiting for file creation: %v", err)
	}

	// Check if writeTestFile finished without any errors
	err = <-writeErrorChan
	if err != nil {
		t.Errorf("Error writing file: %v", err)
	}
}

func TestWaitForFileCreationTimeout(t *testing.T) {
	testDir := path.Join(TempDir, "testBase")

	// Setup the test directory to write the test file
	err := setupTestDirs(testDir)
	defer cleanupTestDirs()
	if err != nil {
		t.Fatalf("Failed to setup test directory: %v", err)
	}

	// Wait for a file to be created, send a short timeout to test
	t.Logf("Testing wait for file creation on path: %v", testDir)
	err = WaitForFileCreation(testDir, "test-file.txt", 1*time.Second, nil)

	// Verify that we received an error because of the timeout
	if err == nil {
		t.Errorf("Expected timeout error, received nil")
	}
}

func TestSliceUtils(t *testing.T) {
	// Test all of the functions on the `slices.go` utils file
	testSlice := []interface{}{"test-item-1", "test-item-2", "test-item-3"}

	// Test slice utils with existing element
	sliceContainsExisting := SliceContains(testSlice, "test-item-2")
	sliceRemoveExisting := SliceRemove(testSlice, "test-item-2")
	removedExistingElementFromSlice := len(testSlice) != len(sliceRemoveExisting)

	// Test slice utils with non existing element
	sliceContainsNew := SliceContains(testSlice, "test-item-4")
	sliceRemoveNew := SliceRemove(testSlice, "test-item-4")
	removedNewElementFromSlice := len(testSlice) != len(sliceRemoveNew)

	sliceTests := []struct {
		testName  string
		want, got bool
	}{
		{"Slice contains existing element", true, sliceContainsExisting},
		{"Slice remove existing element", true, removedExistingElementFromSlice},
		{"Slice contains new element", false, sliceContainsNew},
		{"Slice remove new element", false, removedNewElementFromSlice},
	}

	for _, test := range sliceTests {
		testname := Sprintf("%v,%v,%v", test.testName, test.want, test.got)

		// Run slice subtests
		t.Run(testname, func(t *testing.T) {
			if test.got != test.want {
				t.Errorf("expected request testName %s to be %v, got %v", test.testName, test.got, test.want)
			}
		})
	}

}

// setupTestDirs creates a test directory on the received path
func setupTestDirs(testDir string) error {
	if err := Fs.MkdirAll(testDir, 0777); err != nil {
		return MakeError("failed to create test dir %s: %v", testDir, err)
	}

	return nil
}

// writeTestFile creates a test file on the given directory.
func writeTestFile(testDir string) error {
	filePath := path.Join(testDir, Sprintf("test-file.txt"))
	fileContents := Sprintf("This is test-file with path %s", filePath)

	err := afero.WriteFile(Fs, filePath, []byte(fileContents), 0777)
	if err != nil {
		return MakeError("failed to write to file %s: %v", filePath, err)
	}

	return nil
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return Fs.RemoveAll(WhistDir)
}
