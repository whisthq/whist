package mandelbox

import (
	"os"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/core-go/utils"
	"github.com/whisthq/whist/backend/host-service/mandelbox/portbindings"
)

func TestWriteMandelboxParams(t *testing.T) {
	mandelbox, cancel, goroutineTracker := createTestMandelbox()

	// Defer the wait first since deferred functions are executed in LIFO order.
	defer goroutineTracker.Wait()
	defer cancel()

	if err := mandelbox.AssignPortBindings([]portbindings.PortBinding{
		{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
		{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
		{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
	}); err != nil {
		t.Errorf("Error assigning port bindings: %s", err)
	}

	err := mandelbox.WriteMandelboxParams()
	if err != nil {
		t.Errorf("Error writing mandelbox params: %v", err)
	}

	err = mandelbox.WriteProtocolTimeout(1)
	if err != nil {
		t.Errorf("Error writing protocol timeout: %v", err)
	}

	err = mandelbox.MarkReady()
	if err != nil {
		t.Errorf("Error writing .ready: %v", err)
	}

	var paramsTests = []string{
		"hostPort_for_my_32262_tcp",
		"tty",
		"gpu_index",
		"timeout",
		".ready",
	}

	for _, tt := range paramsTests {
		t.Run(utils.Sprintf("%s", tt), func(t *testing.T) {

			err = verifyResourceMappingFileCreation(tt)
			if err != nil {
				t.Errorf("Could not read file %s: %v", tt, err)
			}
		})
	}
}

func verifyResourceMappingFileCreation(file string) error {
	resourceDir := utils.Sprintf("%s%s/mandelboxResourceMappings/", utils.WhistDir, utils.PlaceholderTestUUID())
	_, err := os.Stat(path.Join(resourceDir, file))
	return err
}
