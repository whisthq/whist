package mandelbox

import (
	"os"
	"path"
	"testing"

	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestWriteMandelboxParams(t *testing.T) {
	mandelbox, cancel, goroutineTracker := createTestMandelboxData()

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

	err = mandelbox.MarkParamsReady()
	if err != nil {
		t.Errorf("Error writing .paramsReady: %v", err)
	}

	err = mandelbox.MarkConfigReady()
	if err != nil {
		t.Errorf("Error writing .configReady: %v", err)
	}

	var paramsTests = []string{
		"hostPort_for_my_32262_tcp",
		"tty",
		"gpu_index",
		"timeout",
		".paramsReady",
		".configReady",
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
	resourceDir := path.Join(utils.WhistDir, utils.PlaceholderTestUUID().String(), "/mandelboxResourceMappings/")
	_, err := os.Stat(path.Join(resourceDir, file))
	return err
}
