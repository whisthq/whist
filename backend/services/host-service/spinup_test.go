package main

import (
	"context"
	"encoding/json"
	"os"
	"path"
	"strconv"
	"sync"
	"testing"

	"github.com/docker/go-connections/nat"
	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

var testMandelboxChrome mandelbox.Mandelbox

func TestStartMandelboxSpinUp(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}

	defer cancel()

	// We always want to start with a clean slate
	uninitializeFilesystem()
	initializeFilesystem()

	dockerClient := mockClient{
		browserImage: utils.MandelboxApp,
	}
	mandelboxID := types.MandelboxID(uuid.New())
	mandelboxDieChan := make(chan bool, 10)
	var appName types.AppName = "chrome"
	testMandelbox, _ := StartMandelboxSpinUp(ctx, cancel, &goroutineTracker, &dockerClient, mandelboxID, appName, mandelboxDieChan)

	// Check that container would have been started
	if !dockerClient.started {
		t.Errorf("Docker container was never started!")
	}

	// Request port bindings for the mandelbox.
	var (
		hostPortForTCP32261, hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
		err32261, err32262, err32263, err32273                                             error
	)

	hostPortForTCP32261, err32261 = testMandelbox.GetHostPort(32261, portbindings.TransportProtocolTCP)
	hostPortForTCP32262, err32262 = testMandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
	hostPortForUDP32263, err32263 = testMandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
	hostPortForTCP32273, err32273 = testMandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
	if err32261 != nil || err32262 != nil || err32263 != nil || err32273 != nil {
		t.Errorf("Couldn't return host port bindings.")
	}

	// Assert port assignments are valid
	portAssignmentAssertions := []struct {
		portName     string
		assignedPort uint16
	}{
		{"HostPortForTCP32261", hostPortForTCP32261},
		{"HostPortForTCP32262", hostPortForTCP32262},
		{"HostPortForTCP32273", hostPortForTCP32273},
		{"HostPortForUDP32263", hostPortForUDP32263},
	}

	for _, portAssertion := range portAssignmentAssertions {
		if portAssertion.assignedPort < portbindings.MinAllowedPort || portAssertion.assignedPort >= portbindings.MaxAllowedPort {
			t.Errorf("Port assignment for %s is invalid: %d", portAssertion.portName, portAssertion.assignedPort)
		}
	}

	// Assert aesKey is valid
	if len(testMandelbox.GetPrivateKey()) != 32 {
		t.Errorf("Expected AES key of length 32, got key: %s of length %d", testMandelbox.GetPrivateKey(), len(testMandelbox.GetPrivateKey()))
	}

	// Check ports have been exposed correctly
	exposedPorts := dockerClient.config.ExposedPorts
	exposedPortNames := []string{"32261/tcp", "32262/tcp", "32273/tcp", "32263/udp"}
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
		{"32261/tcp", hostPortForTCP32261},
		{"32262/tcp", hostPortForTCP32262},
		{"32273/tcp", hostPortForTCP32273},
		{"32263/udp", hostPortForUDP32263},
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
	resourceMappingDir := path.Join(utils.WhistDir, testMandelbox.GetID().String(), "mandelboxResourceMappings")

	hostPortFile := path.Join(resourceMappingDir, "hostPort_for_my_32262_tcp")
	hostPortFileContents, err := os.ReadFile(hostPortFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", hostPortFile, err)
	}
	if string(hostPortFileContents) != utils.Sprintf("%d", hostPortForTCP32262) {
		t.Errorf("Host port resource mapping file does not match returned port.")
	}

	ttyFile := path.Join(resourceMappingDir, "tty")
	ttyFileContents, err := os.ReadFile(ttyFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", ttyFile, err)
	}
	if _, err := strconv.ParseUint(string(ttyFileContents), 10, 8); err != nil {
		t.Errorf("TTY value %s written to file is not a valid uint8: %v.", string(ttyFileContents), err)
	}

	gpuFile := path.Join(resourceMappingDir, "gpu_index")
	gpuFileContents, err := os.ReadFile(gpuFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", gpuFile, err)
	}
	if _, err := strconv.Atoi(string(gpuFileContents)); err != nil {
		t.Errorf("GPU index %s written to file is not a valid int: %v.", string(gpuFileContents), err)
	}

	// Verify that the mandelbox has the connected status to false
	if testMandelbox.GetStatus() != dbdriver.MandelboxStatusWaiting {
		t.Errorf("Mandelbox has invalid connected status: got %v, want %v", testMandelbox.GetStatus(), dbdriver.MandelboxStatusWaiting)
	}
	testMandelboxChrome = testMandelbox
}

func TestFinishMandelboxSpinUp(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}

	var (
		instanceID    types.InstanceID
		testMandelbox mandelbox.Mandelbox
		userID        types.UserID
		err           error
	)

	if metadata.IsRunningInCI() {
		userID = "localdev_host_service_CI"
	} else {
		instanceID = metadata.CloudMetadata.GetInstanceID()
		userID = types.UserID(utils.Sprintf("localdev_host_service_user_%s", instanceID))
	}

	// Get the test mandelbox
	testMandelbox = testMandelboxChrome

	defer cancelMandelboxContextByID(testMandelbox.GetID())
	defer cancel()
	defer uninitializeFilesystem()

	testMandelboxInfo := subscriptions.Mandelbox{
		InstanceID: string(instanceID),
		ID:         testMandelbox.GetID(),
		SessionID:  string(testMandelbox.GetSessionID()),
		UserID:     userID,
	}
	testMandelboxDBEvent := subscriptions.MandelboxEvent{
		Mandelboxes: []subscriptions.Mandelbox{testMandelboxInfo},
	}
	deflatedJSONData, err := configutils.GzipDeflateString(string(`{"data": "test"}`))
	if err != nil {
		t.Fatalf("could not deflate JSON data: %v", err)
	}
	testJSONTransportRequest := httputils.JSONTransportRequest{
		AppName:               types.AppName(utils.MandelboxApp),
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           testMandelbox.GetID(),
		JSONData:              types.JSONData(deflatedJSONData),
		ResultChan:            make(chan httputils.RequestResult),
	}

	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[types.MandelboxID]chan *httputils.JSONTransportRequest)

	dockerClient := mockClient{
		browserImage: utils.MandelboxApp,
	}

	mandelboxSubscription := testMandelboxDBEvent.Mandelboxes[0]

	go handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)
	req, _ := getAppName(mandelboxSubscription, testTransportRequestMap, testmux)
	go FinishMandelboxSpinUp(ctx, cancel, &goroutineTracker, &dockerClient, mandelboxSubscription, testTransportRequestMap, testmux, req)

	// Check that response is as expected
	result := <-testJSONTransportRequest.ResultChan
	if result.Err != nil {
		t.Fatalf("SpinUpMandelbox returned with error: %v", result.Err)
	}
	spinUpResult, ok := result.Result.(httputils.JSONTransportRequestResult)
	if !ok {
		t.Fatalf("Expected instance of SpinUpMandelboxRequestResult, got: %v", result.Result)
	}

	// Check that all resource mapping files were written correctly
	resourceMappingDir := path.Join(utils.WhistDir, testMandelbox.GetID().String(), "mandelboxResourceMappings")

	paramsReadyFile := path.Join(resourceMappingDir, ".paramsReady")
	paramsReadyFileContents, err := os.ReadFile(paramsReadyFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", paramsReadyFile, err)
	}
	if string(paramsReadyFileContents) != ".paramsReady" {
		t.Errorf("Params ready file contains invalid contents: %s", string(paramsReadyFileContents))
	}

	configReadyFile := path.Join(resourceMappingDir, ".configReady")
	configReadyFileContents, err := os.ReadFile(configReadyFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", configReadyFile, err)
	}
	if string(configReadyFileContents) != ".configReady" {
		t.Errorf("Config ready file contains invalid contents: %s", string(configReadyFileContents))
	}

	var jsonData map[string]interface{}
	jsonFile := path.Join(resourceMappingDir, "config.json")
	jsonFileContents, err := os.ReadFile(jsonFile)
	if err != nil {
		t.Fatalf("Failed to read resource file %s: %v", jsonFile, err)
	}
	err = json.Unmarshal(jsonFileContents, &jsonData)
	if err != nil {
		t.Errorf("JSON data file contains invalid contents: %s", string(jsonFileContents))
	}

	// Request port bindings for the mandelbox.
	var (
		hostPortForTCP32261, hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
		err32261, err32262, err32263, err32273                                             error
	)

	hostPortForTCP32261, err32261 = testMandelbox.GetHostPort(32261, portbindings.TransportProtocolTCP)
	hostPortForTCP32262, err32262 = testMandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
	hostPortForUDP32263, err32263 = testMandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
	hostPortForTCP32273, err32273 = testMandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
	if err32261 != nil || err32262 != nil || err32263 != nil || err32273 != nil {
		t.Errorf("Couldn't return host port bindings.")
	}

	if spinUpResult.HostPortForTCP32261 != hostPortForTCP32261 ||
		spinUpResult.HostPortForTCP32262 != hostPortForTCP32262 ||
		spinUpResult.HostPortForUDP32263 != hostPortForUDP32263 ||
		spinUpResult.HostPortForTCP32273 != hostPortForTCP32273 {
		t.Errorf("Ports in the json transport response are incorrect")
	}

	if spinUpResult.AesKey != string(testMandelbox.GetPrivateKey()) {
		t.Errorf("AES key from json transport request is incorrect, got %s, expected %v", spinUpResult.AesKey, testMandelbox.GetPrivateKey())
	}
}
