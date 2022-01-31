package mandelbox

import (
	"context"
	"sync"
	"testing"

	"github.com/spf13/afero"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestNewMandelbox(t *testing.T) {
	mandelbox, cancel, goroutineTracker := createTestMandelboxData()
	oldFs := utils.Fs
	utils.SetFs(afero.NewMemMapFs())

	// Defer the wait first since deferred functions are executed in LIFO order.
	defer goroutineTracker.Wait()
	defer cancel()

	// Check if the mandelbox resource dir was created
	err := verifyResourceMappingFileCreation("")

	if err != nil {
		t.Errorf("error when verifying resource mapping file creation: %v", err)
	}

	// Verify that the fuse mounts are correct
	mandelboxMappings := mandelbox.otherDeviceMappings
	deviceMappings := mandelboxMappings[0]

	if deviceMappings.PathOnHost != "/dev/fuse" {
		t.Errorf("received incorrect PathOnHost: got %s, expected %s", deviceMappings.PathOnHost, "/dev/fuse")
	}
	if deviceMappings.PathInContainer != "/dev/fuse" {
		t.Errorf("received incorrect PathInContainer: got %s, expected %s", deviceMappings.PathInContainer, "/dev/fuse")
	}
	if deviceMappings.CgroupPermissions != "rwm" {
		t.Errorf("received incorrect CgroupPermissions: got %s, expected %s", deviceMappings.CgroupPermissions, "rwm")
	}

	utils.SetFs(oldFs)
}

func TestRegisterCreation(t *testing.T) {
	testUUID := utils.PlaceholderTestUUID().String()

	var registerTests = []struct {
		testName      string
		dockerID      string
		want          types.DockerID
		expectedError bool
	}{
		{"registerCreation with valid DockerID", testUUID, types.DockerID(testUUID), false},
		{"registerCreation with empty DockerID", "", types.DockerID(""), true},
	}

	for _, tt := range registerTests {
		t.Run(tt.testName, func(t *testing.T) {
			mandelbox, cancel, goroutineTracker := createTestMandelboxData()

			// Defer the wait first since deferred functions are executed in LIFO order.
			defer goroutineTracker.Wait()
			defer cancel()

			err := mandelbox.RegisterCreation(types.DockerID(tt.dockerID))
			got := mandelbox.GetDockerID()
			gotErr := err != nil

			if gotErr != tt.expectedError {
				t.Errorf("got error %v, expected error %v", gotErr, tt.expectedError)
			}

			if got != tt.want {
				t.Errorf("got %s, want %s", got, tt.want)
			}
		})
	}
}

func TestSetAppName(t *testing.T) {
	var registerTests = []struct {
		testName      string
		want          types.AppName
		expectedError bool
	}{
		{"setAppName with valid name", types.AppName("test-app-name"), false},
		{"setAppName with empty name", types.AppName(""), true},
	}

	for _, tt := range registerTests {
		t.Run(tt.testName, func(t *testing.T) {
			mandelbox, cancel, goroutineTracker := createTestMandelboxData()

			// Defer the wait first since deferred functions are executed in LIFO order.
			defer goroutineTracker.Wait()
			defer cancel()

			err := mandelbox.SetAppName(tt.want)
			got := mandelbox.GetAppName()
			gotErr := err != nil

			if gotErr != tt.expectedError {
				t.Errorf("got error %s, expected error %s", got, tt.want)
			}

			if got != tt.want {
				t.Errorf("got %s, want %s", got, tt.want)
			}
		})
	}
}

// createTestMandelboxData is a utility function to create a test mandelbox.
func createTestMandelboxData() (*mandelboxData, context.CancelFunc, *sync.WaitGroup) {
	ctx, cancel := context.WithCancel(context.Background())
	routineTracker := sync.WaitGroup{}
	mandelbox := new(ctx, &routineTracker, types.MandelboxID(utils.PlaceholderTestUUID()))

	return mandelbox, cancel, &routineTracker
}
