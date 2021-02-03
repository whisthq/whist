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

	dockercontainer "github.com/docker/docker/api/types/container"

	fractallogger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	fractalhttpserver "github.com/fractal/fractal/ecs-host-service/httpserver"
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

	// Tell the ecs-agent where the host service is listening for HTTP requests
	// so it can pass along mappings between Docker container IDs and FractalIDs,
	// as well as uinput devices
	httpClient := http.Client{
		Timeout: 10 * time.Second,
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
		},
	}
	ecsengine.SetFractalHostServiceMappingSender(
		func(dockerID, fractalID string) error {
			body, err := fractalhttpserver.CreateRegisterDockerContainerIDRequestBody(
				fractalhttpserver.RegisterDockerContainerIDRequest{
					DockerID:  dockerID,
					FractalID: fractalID,
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

	// ecsengine.SetFractalHostServiceUinputDeviceRequester(
	// func(fractalID string) ([]dockercontainer.DeviceMapping, error) {
	// body, err := fractalhttpserver.CreateCreateUinputDevicesRequestBody(
	// fractalhttpserver.CreateUinputDevicesRequest{
	// FractalID: fractalID,
	// },
	// )
	// if err != nil {
	// err := fractallogger.MakeError("Error creating CreateUinputDevicesRequest: %s", err)
	// return nil, err
	// }
	//
	// // We have the request body, now just need to actually make the request
	// requestURL := "https://127.0.0.1" + fractalhttpserver.PortToListen + "/create_uinput_devices"
	//
	// response, err := httpClient.Post(requestURL, "application/json", bytes.NewReader(body))
	// if err != nil {
	// }
	// respbody, err := ioutil.ReadAll(response.Body)
	//
	// var list []dockercontainer.DeviceMapping
	// err = json.Unmarshal(respbody, list)
	//
	// },
	// )

}

func ECSAgentMain() {
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

	// Below is the original main() body from the ECS agent itself.
	ecslogger.InitSeelog()
	os.Exit(ecsapp.Run(os.Args[1:]))
}
