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
	fractalhttpserver "github.com/fractal/fractal/ecs-host-service/httpserver"
)

// UinputDeviceMapping is the type we manipulate in the main package of the
// host service to avoid having to import ecs-agent packages into the main
// package (for modularity).
type UinputDeviceMapping ecsengine.FractalUinputDeviceMapping

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

	// Give the ecs-agent a method to pass along mappings between Docker
	// container IDs and FractalIDs.
	httpClient := http.Client{
		Timeout: 10 * time.Second,
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
		},
	}
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
			requestURL := "https://127.0.0.1" + fractalhttpserver.PortToListen + "/register_docker_container_id"

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
			requestURL := "https://127.0.0.1" + fractalhttpserver.PortToListen + "/create_uinput_devices"

			response, resperr := httpClient.Post(requestURL, "application/json", bytes.NewReader(body))
			if resperr != nil {
				return nil, fractallogger.MakeError("Error returned from /create_uinput_devices endpoint: %s.", resperr)
			}
			respbody, bodyerr := ioutil.ReadAll(response.Body)
			response.Body.Close()
			if bodyerr != nil {
				return nil, fractallogger.MakeError("Error reading response from /create_uinput_devices endpoint: %s", bodyerr)
			}

			fractallogger.Infof("Reponse body: %s", respbody)

			var respstruct struct {
				Result string `json:"result"`
				Error  string `json:"error"`
			}
			err = json.Unmarshal(respbody, &respstruct)
			if err != nil {
				return nil, fractallogger.MakeError("Error unmarshalling response from /create_uinput_devices endpoint. Error: %s, fractalID: %s, response: %s", err, fractalID, respbody)
			}

			var devices []UinputDeviceMapping
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

	// Below is the original main() body from the ECS agent itself. Note that we
	// modify `ecsapp.Run()` to take no arguments instead of reading in
	// `os.Args[1:]`.
	ecslogger.InitSeelog()
	os.Exit(ecsapp.Run([]string{}))
}
