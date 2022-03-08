package main

import (
	"context"
	"io/ioutil"
	"os"
	"path"
	"strconv"
	"sync"
	"testing"
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
	"github.com/docker/go-connections/nat"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// mockClient is a mock Docker client that implements the CommonAPIClient
// interface and can be used as the input to SpinUpMandelbox.
// We are embedding the CommonAPIClient interface inside this struct.
// This will allow us to selectively implement (mock) only the methods we need.
//
// See:
// https://eli.thegreenplace.net/2020/embedding-in-go-part-3-interfaces-in-structs/
type mockClient struct {
	client.CommonAPIClient

	config       container.Config
	hostConfig   container.HostConfig
	browserImage string

	started bool
	stopped bool
}

// ImageList mocks the ImageAPIClient interface's ImageList method and returns
// a single mock image with the tag "testApp"
func (m *mockClient) ImageList(ctx context.Context, options types.ImageListOptions) ([]types.ImageSummary, error) {
	logger.Infof("Called mock ImageList method.")
	testAppImage := types.ImageSummary{
		Containers: 1234,
		Created:    1234,
		ID:         "testImage",
		Labels: map[string]string{
			"test": "test",
		},
		ParentID:    "testParent",
		RepoDigests: []string{"testDigest"},
		RepoTags:    []string{m.browserImage},
		SharedSize:  1234,
		Size:        1234,
		VirtualSize: 1234,
	}
	logger.Infof("%v", options)
	images := []types.ImageSummary{testAppImage}
	return images, nil
}

// ContainerCreate mocks the ContainerAPIClient's ContainerCreate method, saves the inputs,
// and returns a successful container created response.
func (m *mockClient) ContainerCreate(ctx context.Context, config *container.Config, hostConfig *container.HostConfig, networkingConfig *network.NetworkingConfig, platform *v1.Platform, containerName string) (container.ContainerCreateCreatedBody, error) {
	logger.Infof("Called mock ContainerCreate method.")

	m.config = *config
	m.hostConfig = *hostConfig

	createSuccessful := container.ContainerCreateCreatedBody{
		ID:       "testContainer",
		Warnings: []string{},
	}
	return createSuccessful, nil
}

// ContainerStart mocks the ContainerAPIClient's ContainerStart method and returns
// nil, simulating successful container start. The method will sleep for 3 seconds
// to simulate (+ exaggerate) the behaviour of a real container start.
func (m *mockClient) ContainerStart(ctx context.Context, container string, options types.ContainerStartOptions) error {
	logger.Infof("Called mock ContainerStart method.")
	time.Sleep(3 * time.Second)
	m.started = true
	return nil
}

// ContainerStop mocks the ContainerAPIClient's ContainerStop method and returns
// nil, simulating successful container stopping. The method will sleep for 1 seconds
// to simulate (+ exaggerate) the behaviour of a real container stop.
func (m *mockClient) ContainerStop(ctx context.Context, id string, timeout *time.Duration) error {
	logger.Infof("Called mock ContainerStop method.")
	time.Sleep(1 * time.Second)
	m.stopped = true
	return nil
}

// ContainerRemove mocks the ContainerAPIClient's ContainerRemove method and returns
// nil, simulating successful container removal. The method will sleep for 1 seconds
// to simulate (+ exaggerate) the behaviour of a real container being removed.
func (m *mockClient) ContainerRemove(ctx context.Context, id string, options types.ContainerRemoveOptions) error {
	logger.Infof("Called mock ContainerRemove method.")
	time.Sleep(1 * time.Second)
	return nil
}

// TestCreateDockerClient will check if a valid docker client is created without error
func TestCreateDockerClient(t *testing.T) {
	dockerClient, err := createDockerClient()

	if err != nil {
		t.Fatalf("error creating docker client. Error: %v", err)
	}

	if dockerClient == nil {
		t.Fatal("error creating docker client. Expected client, got nil")
	}

	if _, ok := interface{}(dockerClient).(*client.Client); !ok {
		t.Fatalf("error creating docker client. Expected docker client")
	}
}

func TestMandelboxDieHandler(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	dockerClient := mockClient{
		browserImage: "whisthq",
	}

	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)
	testMux := &sync.Mutex{}
	tracker := &sync.WaitGroup{}

	testMux.Lock()
	testTransportRequestMap[mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())] = make(chan *JSONTransportRequest)
	testMux.Unlock()

	m := mandelbox.New(ctx, tracker, mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()))
	m.RegisterCreation(mandelboxtypes.DockerID("test-docker-id"))

	mandelboxDieHandler("test-docker-id", testTransportRequestMap, testMux, &dockerClient)

	// Check transport map to verify mandelbox key was removed.
	testMux.Lock()
	request := testTransportRequestMap[mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())]
	testMux.Unlock()

	if request != nil {
		t.Errorf("Expected mandelbox %v to be removed from JSON transport map, but it still exists.", utils.PlaceholderTestUUID())
	}

	// Check the container stopped method was called successfully.
	if dockerClient.started || !dockerClient.stopped {
		t.Errorf("Expected docker client to be stopped, got started value %v and stopped value %v", dockerClient.started, dockerClient.stopped)
	}

}

func TestInitializeFilesystem(t *testing.T) {
	_, cancel := context.WithCancel(context.Background())
	defer cancel()

	err := os.RemoveAll(utils.WhistDir)
	if err != nil {
		t.Fatalf("Failed to delete directory %s for tests: error: %v\n", utils.WhistPrivateDir, err)
	}

	initializeFilesystem(cancel)

	if _, err := os.Stat(utils.WhistDir); os.IsNotExist(err) {
		t.Errorf("Whist directory was not created by initializeFilesystem")
	}

	if _, err := os.Stat(utils.WhistPrivateDir); os.IsNotExist(err) {
		t.Errorf("Whist private directory was not created by initializeFilesystem")
	}

	if _, err := os.Stat(utils.TempDir); os.IsNotExist(err) {
		t.Errorf("Whist temp directory was not created by initializeFilesystem")
	}
}

func TestUninitializeFilesystem(t *testing.T) {
	uninitializeFilesystem()

	if _, err := os.Stat(utils.WhistDir); os.IsExist(err) {
		t.Errorf("Whist directory was not removed by uninitializeFilesystem")
	}

	if _, err := os.Stat(utils.WhistPrivateDir); os.IsExist(err) {
		t.Errorf("Whist private directory was not removed by uninitializeFilesystem")
	}

	if _, err := os.Stat(utils.TempDir); os.IsExist(err) {
		t.Errorf("Whist temp directory was not removed by uninitializeFilesystem")
	}
}

// TestDrainAndShutdown will check if shutdownInstanceOnExit is set to false
func TestDrainAndShutdown(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	shutdownInstanceOnExit = false

	//  Should update the global variable to be true
	drainAndShutdown(ctx, cancel, nil)

	if !shutdownInstanceOnExit {
		t.Fatalf("error draining and shutting down. Expected shutdownInstanceOnExit = `true`, got `false`")
	}
}

// TestSpinUpMandelbox calls SpinUpMandelbox and checks if the setup steps
// are performed correctly.
func TestSpinUpMandelbox(t *testing.T) {
	var browserImages = []string{"browsers/chrome", "browsers/brave"}
	for _, browserImage := range browserImages {
		t.Run(browserImage, func(t *testing.T) {
			ctx, cancel := context.WithCancel(context.Background())
			goroutineTracker := sync.WaitGroup{}

			// Defer the wait first since deferred functions are executed in LIFO order.
			defer goroutineTracker.Wait()
			defer cancelMandelboxContextByID(mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()))
			defer cancel()

			var instanceID aws.InstanceID
			var userID mandelboxtypes.UserID
			var err error
			if metadata.IsRunningInCI() {
				userID = "localdev_host_service_CI"
			} else {
				instanceID, err = aws.GetInstanceID()
				if err != nil {
					logger.Errorf("Can't get AWS Instance name for localdev user config userID.")
				}
				userID = mandelboxtypes.UserID(utils.Sprintf("localdev_host_service_user_%s", instanceID))
			}

			// We always want to start with a clean slate
			uninitializeFilesystem()
			initializeFilesystem(cancel)
			defer uninitializeFilesystem()

			testMandelboxInfo := subscriptions.Mandelbox{
				InstanceID: string(instanceID),
				ID:         mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
				SessionID:  "1234",
				UserID:     userID,
			}
			testMandelboxDBEvent := subscriptions.MandelboxEvent{
				Mandelboxes: []subscriptions.Mandelbox{testMandelboxInfo},
			}
			testJSONTransportRequest := JSONTransportRequest{
				AppName:               mandelboxtypes.AppName(browserImage),
				ConfigEncryptionToken: "testToken1234",
				JwtAccessToken:        "test_jwt_token",
				MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
				JSONData:              "test_json_data",
				resultChan:            make(chan httputils.RequestResult),
			}

			testmux := &sync.Mutex{}
			testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)

			dockerClient := mockClient{
				browserImage: browserImage,
			}

			go handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)
			go SpinUpMandelbox(ctx, cancel, &goroutineTracker, &dockerClient, &testMandelboxDBEvent, testTransportRequestMap, testmux)

			// Check that response is as expected
			result := <-testJSONTransportRequest.resultChan
			if result.Err != nil {
				t.Fatalf("SpinUpMandelbox returned with error: %v", result.Err)
			}
			spinUpResult, ok := result.Result.(JSONTransportRequestResult)
			if !ok {
				t.Fatalf("Expected instance of SpinUpMandelboxRequestResult, got: %v", result.Result)
			}

			// Check that container would have been started
			if !dockerClient.started {
				t.Errorf("Docker container was never started!")
			}

			// Assert port assignments are valid
			portAssignmentAssertions := []struct {
				portName     string
				assignedPort uint16
			}{
				{"HostPortForTCP32262", spinUpResult.HostPortForTCP32262},
				{"HostPortForTCP32273", spinUpResult.HostPortForTCP32273},
				{"HostPortForUDP32263", spinUpResult.HostPortForUDP32263},
			}

			for _, portAssertion := range portAssignmentAssertions {
				if portAssertion.assignedPort < portbindings.MinAllowedPort || portAssertion.assignedPort >= portbindings.MaxAllowedPort {
					t.Errorf("Port assignment for %s is invalid: %d", portAssertion.portName, portAssertion.assignedPort)
				}
			}

			// Assert aesKey is valid
			if len(spinUpResult.AesKey) != 32 {
				t.Errorf("Expected AES key of length 32, got key: %s of length %d", spinUpResult.AesKey, len(spinUpResult.AesKey))
			}

			// Check that container config was passed in correctly
			if dockerClient.config.Image != browserImage {
				t.Errorf("Expected container to use image %s, got %v", browserImage, dockerClient.config.Image)
			}

			// Check ports have been exposed correctly
			exposedPorts := dockerClient.config.ExposedPorts
			exposedPortNames := []string{"32262/tcp", "32273/tcp", "32263/udp"}
			for _, exposedPort := range exposedPortNames {
				if _, ok := exposedPorts[nat.Port(exposedPort)]; !ok {
					t.Errorf("Expected port %s to be exposed on docker container, but it wasn't", exposedPort)
				}
			}

			// Check that host config port bindings are correct
			portBindings := dockerClient.hostConfig.PortBindings
			portBindingAssertions := []struct {
				portName     string
				assignedPort uint16
			}{
				{"32262/tcp", spinUpResult.HostPortForTCP32262},
				{"32273/tcp", spinUpResult.HostPortForTCP32273},
				{"32263/udp", spinUpResult.HostPortForUDP32263},
			}

			for _, portBindingAssertion := range portBindingAssertions {
				binding, ok := portBindings[nat.Port(portBindingAssertion.portName)]
				if !ok {
					t.Errorf("Expected port %s to be bound, but it wasn't", portBindingAssertion.portName)
				}
				if len(binding) < 1 || binding[0].HostPort != strconv.Itoa(int(portBindingAssertion.assignedPort)) {
					t.Errorf("Expected port %s to be bound to %d, but it wasn't", portBindingAssertion.portName, portBindingAssertion.assignedPort)
				}
			}

			// Check that all resource mapping files were written correctly
			resourceMappingDir := path.Join(utils.WhistDir, utils.PlaceholderTestUUID().String(), "mandelboxResourceMappings")

			hostPortFile := path.Join(resourceMappingDir, "hostPort_for_my_32262_tcp")
			hostPortFileContents, err := ioutil.ReadFile(hostPortFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", hostPortFile, err)
			}
			if string(hostPortFileContents) != utils.Sprintf("%d", spinUpResult.HostPortForTCP32262) {
				t.Errorf("Host port resource mapping file does not match returned port.")
			}

			ttyFile := path.Join(resourceMappingDir, "tty")
			ttyFileContents, err := ioutil.ReadFile(ttyFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", ttyFile, err)
			}
			if _, err := strconv.ParseUint(string(ttyFileContents), 10, 8); err != nil {
				t.Errorf("TTY value %s written to file is not a valid uint8: %v.", string(ttyFileContents), err)
			}

			gpuFile := path.Join(resourceMappingDir, "gpu_index")
			gpuFileContents, err := ioutil.ReadFile(gpuFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", gpuFile, err)
			}
			if _, err := strconv.Atoi(string(gpuFileContents)); err != nil {
				t.Errorf("GPU index %s written to file is not a valid int: %v.", string(gpuFileContents), err)
			}

			paramsReadyFile := path.Join(resourceMappingDir, ".paramsReady")
			paramsReadyFileContents, err := ioutil.ReadFile(paramsReadyFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", paramsReadyFile, err)
			}
			if string(paramsReadyFileContents) != ".paramsReady" {
				t.Errorf("Params ready file contains invalid contents: %s", string(paramsReadyFileContents))
			}

			configReadyFile := path.Join(resourceMappingDir, ".configReady")
			configReadyFileContents, err := ioutil.ReadFile(configReadyFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", configReadyFile, err)
			}
			if string(configReadyFileContents) != ".configReady" {
				t.Errorf("Config ready file contains invalid contents: %s", string(configReadyFileContents))
			}
		})
	}
}

// cancelMandelboxContextByID tries to look up the mandelbox by its ID
// and cancels the context associated with it.
func cancelMandelboxContextByID(id mandelboxtypes.MandelboxID) error {
	mandelbox, err := mandelbox.LookUpByMandelboxID(id)
	if err != nil {
		return utils.MakeError("could not cancel mandelbox context: %v", err)
	}
	mandelbox.Close()
	return nil
}
