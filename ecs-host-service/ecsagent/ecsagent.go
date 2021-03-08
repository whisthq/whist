package ecsagent

import (
	"bufio"
	"bytes"
	"crypto/tls"
	"encoding/json"
	"io/ioutil"
	"math/rand"
	"net/http"
	"os"
	"strings"
	"time"

	ecsapp "github.com/fractal/ecs-agent/agent/app"
	ecsengine "github.com/fractal/ecs-agent/agent/engine"
	ecslogger "github.com/fractal/ecs-agent/agent/logger"

	fractallogger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	fractaltypes "github.com/fractal/fractal/ecs-host-service/fractaltypes"
	fractalhttpserver "github.com/fractal/fractal/ecs-host-service/httpserver"

	"github.com/cihub/seelog"
)

func init() {
	rand.Seed(time.Now().UnixNano())

	// If we don't make this directory, the ecs-agent crashes.
	err := os.MkdirAll("/var/lib/ecs/data/", 0755)
	if err != nil {
		fractallogger.Panicf("Could not make directory /var/lib/ecs/data: Error: %s", err)
	}

	err = os.MkdirAll("/var/log/ecs/", 0755)
	if err != nil {
		fractallogger.Panicf("Could not make directory /var/log/ecs: Error: %s", err)
	}

	err = os.MkdirAll("/etc/ecs/", 0755)
	if err != nil {
		fractallogger.Panicf("Could not make directory /etc/ecs: Error: %s", err)
	}

	// Create an HTTP client for the following functions
	httpClient := http.Client{
		Timeout: 10 * time.Second,
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
		},
	}

	// Give the ecs-agent a method to pass along mappings between Docker
	// container IDs and FractalIDs.
	ecsengine.SetFractalHostServiceMappingSender(
		func(dockerID, fractalID, appName string) error {
			body, err := fractalhttpserver.CreateRegisterDockerContainerIDRequestBody(
				fractalhttpserver.RegisterDockerContainerIDRequest{
					DockerID:  dockerID,
					FractalID: fractalID,
					AppName:   appName,
				},
			)
			if err != nil {
				err := fractallogger.MakeError("Error creating RegisterDockerContainerIDRequest: %s", err)
				fractallogger.Error(err)
				return err
			}

			// We have the request body, now just need to actually make the request
			requestURL := "https://127.0.0.1:" + fractallogger.Sprintf("%v", fractalhttpserver.PortToListen) + "/register_docker_container_id"

			_, err = httpClient.Post(requestURL, "application/json", bytes.NewReader(body))
			return err
		},
	)

	// Give the ecs-agent a method to ask for uinput devices for a container.
	ecsengine.SetFractalHostServiceUinputDeviceRequester(
		func(fractalID string) ([]ecsengine.FractalUinputDeviceMapping, error) {
			body, err := fractalhttpserver.CreateCreateUinputDevicesRequestBody(
				fractalhttpserver.CreateUinputDevicesRequest{
					FractalID: fractalID,
				},
			)
			if err != nil {
				err := fractallogger.MakeError("Error creating CreateUinputDevicesRequest: %s", err)
				return nil, err
			}

			// We have the request body, now just need to actually make the request
			requestURL := "https://127.0.0.1:" + fractallogger.Sprintf("%v", fractalhttpserver.PortToListen) + "/create_uinput_devices"

			response, err := httpClient.Post(requestURL, "application/json", bytes.NewReader(body))
			if err != nil {
				return nil, fractallogger.MakeError("Error returned from /create_uinput_devices endpoint: %s", err)
			}
			respbody, err := ioutil.ReadAll(response.Body)
			if err != nil {
				return nil, fractallogger.MakeError("Error reading response.Body from /create_uinput_devices endpoint: %s", err)
			}
			response.Body.Close()

			fractallogger.Infof("Reponse body from /create_uinput_devices endpoint: %s", respbody)

			var respstruct struct {
				Result string `json:"result"`
				Error  string `json:"error"`
			}
			err = json.Unmarshal(respbody, &respstruct)
			if err != nil {
				return nil, fractallogger.MakeError("Error unmarshalling response from /create_uinput_devices endpoint. Error: %s, fractalID: %s, response: %s", err, fractalID, respbody)
			}

			var devices []fractaltypes.UinputDeviceMapping
			err = json.Unmarshal([]byte(respstruct.Result), &devices)
			if err != nil {
				return nil, fractallogger.MakeError("Error unmarshalling devices from /create_uinput_devices endpoint. Error: %s, fractalID: %s, device list: %s", err, fractalID, respstruct.Result)
			}

			// Convert to the right type
			result := make([]ecsengine.FractalUinputDeviceMapping, len(devices))
			for i, v := range devices {
				result[i] = ecsengine.FractalUinputDeviceMapping(v)
			}

			return result, nil
		},
	)

	// Give the ecs-agent a method to ask for port bindings for a container.
	ecsengine.SetFractalPortBindingRequester(
		func(fractalID string, desiredPorts []ecsengine.FractalPortBinding) ([]ecsengine.FractalPortBinding, error) {
			convertBindingSlicetoFractalType := func(x []ecsengine.FractalPortBinding) []fractaltypes.PortBinding {
				result := make([]fractaltypes.PortBinding, len(x))
				for i, v := range x {
					result[i] = fractaltypes.PortBinding(v)
				}
				return result
			}

			convertBindingSliceToECSType := func(x []fractaltypes.PortBinding) []ecsengine.FractalPortBinding {
				result := make([]ecsengine.FractalPortBinding, len(x))
				for i, v := range x {
					result[i] = ecsengine.FractalPortBinding(v)
				}
				return result
			}

			body, err := fractalhttpserver.CreateRequestPortBindingsRequestBody(
				fractalhttpserver.RequestPortBindingsRequest{
					FractalID: fractalID,
					Bindings:  convertBindingSlicetoFractalType(desiredPorts),
				},
			)
			if err != nil {
				err := fractallogger.MakeError("Error creating RequestPortBindingsRequest: %s", err)
				return nil, err
			}

			// We have the request body, now just need to actually make the request
			requestURL := "https://127.0.0.1:" + fractallogger.Sprintf("%v", fractalhttpserver.PortToListen) + "/request_port_bindings"

			response, err := httpClient.Post(requestURL, "application/json", bytes.NewReader(body))
			if err != nil {
				return nil, fractallogger.MakeError("Error returned from /request_port_bindings endpoint: %s", err)
			}
			respbody, err := ioutil.ReadAll(response.Body)
			if err != nil {
				return nil, fractallogger.MakeError("Error reading response.Body from /request_port_bindings endpoint: %s", err)
			}
			response.Body.Close()

			fractallogger.Infof("Reponse body from /request_port_bindings endpoint: %s", respbody)

			var respstruct struct {
				Result string `json:"result"`
				Error  string `json:"error"`
			}
			err = json.Unmarshal(respbody, &respstruct)
			if err != nil {
				return nil, fractallogger.MakeError("Error unmarshalling response from /request_port_bindings endpoint. Error: %s, fractalID: %s, response: %s", err, fractalID, respbody)
			}

			var bindings []fractaltypes.PortBinding
			err = json.Unmarshal([]byte(respstruct.Result), &bindings)
			if err != nil {
				return nil, fractallogger.MakeError("Error unmarshalling bindings from /request_port_bindings endpoint. Error: %s, fractalID: %s, bindings: %s", err, fractalID, respstruct.Result)
			}

			return convertBindingSliceToECSType(bindings), nil
		},
	)

}

// Main is the function that actually starts the ecsagent. It also contains the
// main function from fractal/ecs-agent.
func Main() {
	// We want to set the relevant environment variables here so that the ECS
	// agent is configured correctly before it actually runs.

	// Read in the ECS config file
	ecsConfigFilePath := "/etc/ecs/ecs.config"
	file, err := os.Open(ecsConfigFilePath)
	if err != nil {
		fractallogger.Panicf("Couldn't open ECS Config File %s. Error: %s", ecsConfigFilePath, err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}

		// We split on the first `=` only
		substrs := strings.SplitN(line, "=", 2)
		if len(substrs) != 2 {
			fractallogger.Panicf("Couldn't parse ECS Config file line: %s", line)
		}

		// Actually set the environment variable
		err := os.Setenv(substrs[0], substrs[1])
		if err != nil {
			fractallogger.Panicf("Couldn't set environment variable %s to value %s. Error: %s", substrs[0], substrs[1], err)
		} else {
			fractallogger.Infof("Set environment variable %s to value %s.", substrs[0], substrs[1])
		}
	}

	fractallogger.Infof("Successfully set all env vars from %s", ecsConfigFilePath)

	seelogBridge, err := seelog.LoggerFromCustomReceiver(&trivialSeeLogReceiver{})
	if err != nil {
		fractallogger.Panicf("Error initializing seelogBridge: %s", err)
	}

	// Below is the original main() body from the ECS agent itself. Note that we
	// modify `ecsapp.Run()` to take no arguments instead of reading in
	// `os.Args[1:]`.
	ecslogger.InitSeelog(seelogBridge)
	os.Exit(ecsapp.Run([]string{}))
}
