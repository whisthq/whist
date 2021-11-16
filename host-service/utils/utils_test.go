package utils

import (
	"math/rand"
	"os"
	"path"
	"sync"
	"testing"
	"time"
)

func TestPlaceholderUUIDs(t *testing.T) {
	testMap := []struct {
		key       string
		want, got string
	}{
		{"Warmup UUID", "11111111-1111-1111-1111-111111111111", PlaceholderWarmupUUID().String()},
		{"Warmup UUID", "22222222-2222-2222-2222-222222222222", PlaceholderTestUUID().String()},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}
}

func TestWaitWithDebugPrints(t *testing.T) {
	wg := sync.WaitGroup{}
	timeout := 1 * time.Second
	level := 2

	for i := 1; i <= 5; i++ {
		wg.Add(1)

		go func() {
			defer wg.Done()
			r := rand.Intn(5)
			time.Sleep(time.Duration(r) * time.Second)
		}()
	}
	WaitWithDebugPrints(&wg, timeout, level)
	wg.Wait()
}

func TestWaitForFileCreation(t *testing.T) {
	testDir := path.Join(TempDir, "testBase")

	err := setupTestDirs(testDir)
	defer cleanupTestDirs()

	if err != nil {
		t.Errorf("Failed to setup test directory: %v", err)
	}

	waitErrorChan := make(chan error)
	go func() {
		t.Logf("Testing wait for file creation on path: %v", testDir)
		err := WaitForFileCreation(testDir, "test-file.txt", 10*time.Second)
		waitErrorChan <- err
	}()

	writeErrorChan := make(chan error)
	go func() {
		err := writeTestFile(testDir)
		writeErrorChan <- err
	}()

	err = <-waitErrorChan
	if err != nil {
		t.Errorf("Error waiting for file creation: %v", err)
	}

	err = <-writeErrorChan
	if err != nil {
		t.Errorf("Error writing file: %v", err)
	}
}

func TestSliceUtils(t *testing.T) {
	testSlice := []interface{}{"test-item-1", "test-item-2", "test-item-3"}

	// Test slice utils with existing element
	sliceContainsExisting := SliceContains(testSlice, "test-item-2")
	sliceRemoveExisting := SliceRemove(testSlice, "test-item-2")
	removedExistingElementFromSlice := len(testSlice) != len(sliceRemoveExisting)

	// Test slice utils with non existing element
	sliceContainsNew := SliceContains(testSlice, "test-item-4")
	sliceRemoveNew := SliceRemove(testSlice, "test-item-4")
	removedNewElementFromSlice := len(testSlice) != len(sliceRemoveNew)

	testMap := []struct {
		key       string
		want, got bool
	}{
		{"Slice contains existing element", true, sliceContainsExisting},
		{"Slice remove existing element", true, removedExistingElementFromSlice},
		{"Slice contains new element", false, sliceContainsNew},
		{"Slice remove new element", false, removedNewElementFromSlice},
	}

	for _, test := range testMap {
		testname := Sprintf("%v,%v,%v", test.key, test.want, test.got)
		t.Run(testname, func(t *testing.T) {
			if test.got != test.want {
				t.Errorf("expected request key %s to be %v, got %v", test.key, test.got, test.want)
			}
		})
	}

}

func TestStringUtils(t *testing.T) {
	rand.Seed(2)
	hex := RandHex(16)
	t.Log(hex)
}

// setupTestDirs creates a sample user config with some nested directories
// and files inside.
func setupTestDirs(testDir string) error {
	if err := os.MkdirAll(testDir, 0777); err != nil {
		return MakeError("failed to create config dir %s: %v", testDir, err)
	}

	return nil
}

func writeTestFile(testDir string) error {
	filePath := path.Join(testDir, Sprintf("test-file.txt"))
	fileContents := Sprintf("This is test-file with path %s", filePath)

	err := os.WriteFile(filePath, []byte(fileContents), 0777)
	if err != nil {
		return MakeError("failed to write to file %s: %v", filePath, err)
	}

	return nil
}

// cleanupTestDirs removes the created directories created by the integration
// test. This allows the test to be runnable multiple times without
// creation errors.
func cleanupTestDirs() error {
	return os.RemoveAll(FractalDir)
}
