package main

import (
	"context"
	"io/ioutil"
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
	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/mandelbox/portbindings"
	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/metadata/aws"
	"github.com/fractal/fractal/host-service/subscriptions"
	"github.com/fractal/fractal/host-service/utils"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"
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

	config     container.Config
	hostConfig container.HostConfig

	started bool
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
		RepoTags:    []string{"testApp"},
		SharedSize:  1234,
		Size:        1234,
		VirtualSize: 1234,
	}
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

// TestSpinUpMandelbox calls SpinUpMandelbox and checks if the setup steps
// are performed correctly.
func TestSpinUpMandelbox(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}

	var instanceName aws.InstanceName
	var userID mandelboxtypes.UserID
	var err error
	if metadata.IsRunningInCI() {
		userID = "localdev_host_service_CI"
	} else {
		instanceName, err = aws.GetInstanceName()
		if err != nil {
			logger.Errorf("Can't get AWS Instance name for localdev user config userID.")
		}
		userID = mandelboxtypes.UserID(utils.Sprintf("localdev_host_service_user_%s", instanceName))
	}

	// We always want to start with a clean slate
	uninitializeFilesystem()
	initializeFilesystem(cancel)
	defer uninitializeFilesystem()

	testMandelboxInfo := subscriptions.Mandelbox{
		InstanceName: string(instanceName),
		MandelboxID:  "testMandelbox",
		SessionID:    1234,
		UserID:       userID,
	}
	testMandelboxDBEvent := subscriptions.MandelboxInfoEvent{
		MandelboxInfo: []subscriptions.Mandelbox{testMandelboxInfo},
	}
	testJSONTransportRequest := JSONTransportRequest{
		ConfigEncryptionToken: "testToken1234",
		JwtAccessToken:        "test_jwt_token",
		JSONData:              "test_json_data",
		resultChan:            make(chan requestResult),
	}

	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.UserID]chan *JSONTransportRequest)

	dockerClient := mockClient{}
	go handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)
	go SpinUpMandelbox(ctx, cancel, &goroutineTracker, &dockerClient, &testMandelboxDBEvent, testTransportRequestMap, testmux)

	goroutineTracker.Wait()

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
	if spinUpResult.HostPortForTCP32262 < portbindings.MinAllowedPort || spinUpResult.HostPortForTCP32262 >= portbindings.MaxAllowedPort {
		t.Errorf("HostPortForTCP32262 is invalid: %d", spinUpResult.HostPortForTCP32262)
	}
	if spinUpResult.HostPortForTCP32273 < portbindings.MinAllowedPort || spinUpResult.HostPortForTCP32273 >= portbindings.MaxAllowedPort {
		t.Errorf("HostPortForTCP32273 is invalid: %d", spinUpResult.HostPortForTCP32273)
	}
	if spinUpResult.HostPortForUDP32263 < portbindings.MinAllowedPort || spinUpResult.HostPortForUDP32263 >= portbindings.MaxAllowedPort {
		t.Errorf("HostPortForUDP32263 is invalid: %d", spinUpResult.HostPortForUDP32263)
	}

	// Assert aesKey is valid
	if len(spinUpResult.AesKey) != 32 {
		t.Errorf("Expected AES key of length 32, got key: %s of length %d", spinUpResult.AesKey, len(spinUpResult.AesKey))
	}

	// Check that container config was passed in correctly
	if dockerClient.config.Image != "testApp" {
		t.Errorf("Expected container to use image testApp, got %v", dockerClient.config.Image)
	}

	exposedPorts := dockerClient.config.ExposedPorts
	if _, ok := exposedPorts[nat.Port("32262/tcp")]; !ok {
		t.Error("Port 32262/tcp is not exposed on docker container.")
	}
	if _, ok := exposedPorts[nat.Port("32263/udp")]; !ok {
		t.Error("Port 32263/udp is not exposed on docker container.")
	}
	if _, ok := exposedPorts[nat.Port("32273/tcp")]; !ok {
		t.Error("Port 32273/tcp is not exposed on docker container.")
	}

	// Check that host config port bindings are correct
	portBindings := dockerClient.hostConfig.PortBindings

	binding32262, ok := portBindings[nat.Port("32262/tcp")]
	if !ok {
		t.Error("Port 32262/tcp is not bound.")
	}
	if len(binding32262) < 1 || binding32262[0].HostPort != utils.Sprintf("%d", spinUpResult.HostPortForTCP32262) {
		t.Error("Binding for port 32262 does not match returned result.")
	}

	binding32263, ok := portBindings[nat.Port("32263/udp")]
	if !ok {
		t.Error("Port 32263/udp is not bound.")
	}
	if len(binding32263) < 1 || binding32263[0].HostPort != utils.Sprintf("%d", spinUpResult.HostPortForUDP32263) {
		t.Error("Binding for port 32263 does not match returned result.")
	}

	binding32273, ok := portBindings[nat.Port("32273/tcp")]
	if !ok {
		t.Error("Port 32273/tcp is not bound.")
	}
	if len(binding32273) < 1 || binding32273[0].HostPort != utils.Sprintf("%d", spinUpResult.HostPortForTCP32273) {
		t.Error("Binding for port 32273 does not match returned result.")
	}

	// Check that all resource mapping files were written correctly
	resourceMappingDir := path.Join(utils.FractalDir, "testMandelbox", "mandelboxResourceMappings")

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

	readyFile := path.Join(resourceMappingDir, ".ready")
	readyFileContents, err := ioutil.ReadFile(readyFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", readyFile, err)
	}
	if string(readyFileContents) != ".ready" {
		t.Errorf("Ready file contains invalid contents: %s", string(readyFileContents))
	}
}
