package ecsagent

import (
	"bufio"
	"math/rand"
	"os"
	"strings"
	"time"

	ecsapp "github.com/fractal/ecs-agent/agent/app"
	ecslogger "github.com/fractal/ecs-agent/agent/logger"

	fractallogger "github.com/fractal/fractal/ecs-host-service/fractallogger"

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
