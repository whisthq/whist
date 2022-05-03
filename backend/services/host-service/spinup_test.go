package main

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"path"
	"strconv"
	"sync"
	"testing"

	"github.com/docker/go-connections/nat"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

var (
	testMandelboxChrome mandelbox.Mandelbox
	testMandelboxBrave  mandelbox.Mandelbox
)

func TestStartMandelboxSpinUp(t *testing.T) {
	var browserImages = []string{"browsers/chrome", "browsers/brave"}

	for _, browserImage := range browserImages {
		t.Run(browserImage, func(t *testing.T) {
			ctx, cancel := context.WithCancel(context.Background())
			goroutineTracker := sync.WaitGroup{}

			// Defer the wait first since deferred functions are executed in LIFO order.
			defer goroutineTracker.Wait()
			defer cancelMandelboxContextByID(mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()))
			defer cancel()

			// We always want to start with a clean slate
			uninitializeFilesystem()
			initializeFilesystem(cancel)
			defer uninitializeFilesystem()

			dockerClient := mockClient{
				browserImage: browserImage,
			}

			testMandelbox := StartMandelboxSpinUp(ctx, cancel, &goroutineTracker, &dockerClient)

			// Check that container would have been started
			if !dockerClient.started {
				t.Errorf("Docker container was never started!")
			}

			// Request port bindings for the mandelbox.
			var (
				hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
				err32262, err32263, err32273                                  error
			)

			hostPortForTCP32262, err32262 = testMandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
			hostPortForUDP32263, err32263 = testMandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
			hostPortForTCP32273, err32273 = testMandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
			if err32262 != nil || err32263 != nil || err32273 != nil {
				t.Errorf("Couldn't return host port bindings.")
			}

			// Assert port assignments are valid
			portAssignmentAssertions := []struct {
				portName     string
				assignedPort uint16
			}{
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
			if len(testMandelbox.GetAESKey()) != 32 {
				t.Errorf("Expected AES key of length 32, got key: %s of length %d", testMandelbox.GetAESKey(), len(testMandelbox.GetAESKey()))
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
			hostPortFileContents, err := ioutil.ReadFile(hostPortFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", hostPortFile, err)
			}
			if string(hostPortFileContents) != utils.Sprintf("%d", hostPortForTCP32262) {
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

			// Verify that the mandelbox has the connected status to false
			if testMandelbox.GetConnectedStatus() {
				t.Errorf("Mandelbox has invalid connected status: got true, want false")
			}

			if browserImage == "browsers/chrome" {
				testMandelboxChrome = testMandelbox
			} else {
				testMandelboxBrave = testMandelbox
			}
		})
	}
}

func TestFinishMandelboxSpinUp(t *testing.T) {
	var browserImages = []string{"browsers/chrome", "browsers/brave"}

	for _, browserImage := range browserImages {
		t.Run(browserImage, func(t *testing.T) {
			ctx, cancel := context.WithCancel(context.Background())
			goroutineTracker := sync.WaitGroup{}

			var (
				instanceID    aws.InstanceID
				testMandelbox mandelbox.Mandelbox
			)

			// Get the test mandelbox for each app
			if browserImage == "browsers/chrome" {
				testMandelbox = testMandelboxChrome
			} else {
				testMandelbox = testMandelboxBrave
			}

			// Defer the wait first since deferred functions are executed in LIFO order.
			defer goroutineTracker.Wait()
			defer cancelMandelboxContextByID(testMandelbox.GetID())
			defer cancel()

			// We always want to start with a clean slate
			uninitializeFilesystem()
			initializeFilesystem(cancel)
			defer uninitializeFilesystem()

			instanceID, err := aws.GetInstanceID()
			if err != nil {
				logger.Errorf("Can't get AWS Instance name for localdev user config userID.")
			}

			testMandelboxInfo := subscriptions.Mandelbox{
				InstanceID: string(instanceID),
				ID:         testMandelbox.GetID(),
				SessionID:  string(testMandelbox.GetSessionID()),
				UserID:     testMandelbox.GetUserID(),
			}
			testMandelboxDBEvent := subscriptions.MandelboxEvent{
				Mandelboxes: []subscriptions.Mandelbox{testMandelboxInfo},
			}
			testJSONTransportRequest := JSONTransportRequest{
				AppName:               mandelboxtypes.AppName(browserImage),
				ConfigEncryptionToken: "testToken1234",
				JwtAccessToken:        "test_jwt_token",
				MandelboxID:           testMandelbox.GetID(),
				JSONData:              `{"data": "test"}`,
				resultChan:            make(chan httputils.RequestResult),
			}

			testmux := &sync.Mutex{}
			testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)

			dockerClient := mockClient{
				browserImage: browserImage,
			}

			go handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)
			go FinishMandelboxSpinUp(ctx, cancel, &goroutineTracker, &dockerClient, &testMandelboxDBEvent, testTransportRequestMap, testmux)

			// Check that response is as expected
			result := <-testJSONTransportRequest.resultChan
			if result.Err != nil {
				t.Fatalf("SpinUpMandelbox returned with error: %v", result.Err)
			}
			spinUpResult, ok := result.Result.(JSONTransportRequestResult)
			if !ok {
				t.Fatalf("Expected instance of SpinUpMandelboxRequestResult, got: %v", result.Result)
			}

			// Check that all resource mapping files were written correctly
			resourceMappingDir := path.Join(utils.WhistDir, testMandelbox.GetID().String(), "mandelboxResourceMappings")

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

			// Verify config json was written correctly
			resourceMappingDir := path.Join(utils.WhistDir, testMandelbox.GetID().String(), "mandelboxResourceMappings")

			var jsonData map[string]interface{}
			jsonFile := path.Join(resourceMappingDir, "config.json")
			jsonFileContents, err := ioutil.ReadFile(jsonFile)
			if err != nil {
				t.Fatalf("Failed to read resource file %s: %v", jsonFile, err)
			}
			err = json.Unmarshal(jsonFileContents, &jsonData)
			if err != nil {
				t.Errorf("JSON data file contains invalid contents: %s", string(jsonFileContents))
			}

			// Request port bindings for the mandelbox.
			var (
				hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
				err32262, err32263, err32273                                  error
			)

			hostPortForTCP32262, err32262 = testMandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
			hostPortForUDP32263, err32263 = testMandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
			hostPortForTCP32273, err32273 = testMandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
			if err32262 != nil || err32263 != nil || err32273 != nil {
				t.Errorf("Couldn't return host port bindings.")
			}

			if spinUpResult.HostPortForTCP32262 != hostPortForTCP32262 ||
				spinUpResult.HostPortForUDP32263 != hostPortForUDP32263 ||
				spinUpResult.HostPortForTCP32273 != hostPortForTCP32273 {
				t.Errorf("Ports in the json transport response are incorrect")
			}

			if spinUpResult.AesKey != string(testMandelbox.GetAESKey()) {
				t.Errorf("AES key from json transport request is incorrect, got %s, expected %v", spinUpResult.AesKey, testMandelbox.GetAESKey())
			}
		})
	}
}
