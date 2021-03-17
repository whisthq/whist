package ecsagent // import "github.com/fractal/fractal/ecs-host-service/ecsagent"

import (
	"bufio"
	"context"
	"os"
	"strings"
	"sync"

	ecsapp "github.com/fractal/ecs-agent/agent/app"
	ecslogger "github.com/fractal/ecs-agent/agent/logger"

	fractallogger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	"github.com/cihub/seelog"
)

// Main is the function that actually starts the ecsagent. It also contains the
// main function from fractal/ecs-agent.
func Main(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	// If we don't make this directory, the ecs-agent crashes.
	err := os.MkdirAll("/var/lib/ecs/data/", 0755)
	if err != nil {
		fractallogger.Panicf(globalCancel, "Could not make directory /var/lib/ecs/data: Error: %s", err)
	}

	err = os.MkdirAll("/var/log/ecs/", 0755)
	if err != nil {
		fractallogger.Panicf(globalCancel, "Could not make directory /var/log/ecs: Error: %s", err)
	}

	err = os.MkdirAll("/etc/ecs/", 0755)
	if err != nil {
		fractallogger.Panicf(globalCancel, "Could not make directory /etc/ecs: Error: %s", err)
	}

	// We want to set the relevant environment variables here so that the ECS
	// agent is configured correctly before it actually runs.

	// Read in the ECS config file
	ecsConfigFilePath := "/etc/ecs/ecs.config"
	file, err := os.Open(ecsConfigFilePath)
	if err != nil {
		fractallogger.Panicf(globalCancel, "Couldn't open ECS Config File %s. Error: %s", ecsConfigFilePath, err)
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
			fractallogger.Panicf(globalCancel, "Couldn't parse ECS Config file line: %s", line)
		}

		// Actually set the environment variable
		err := os.Setenv(substrs[0], substrs[1])
		if err != nil {
			fractallogger.Panicf(globalCancel, "Couldn't set environment variable %s to value %s. Error: %s", substrs[0], substrs[1], err)
		} else {
			fractallogger.Infof("Set environment variable %s to value %s.", substrs[0], substrs[1])
		}
	}

	fractallogger.Infof("Successfully set all env vars from %s", ecsConfigFilePath)

	seelogBridge, err := seelog.LoggerFromCustomReceiver(&trivialSeeLogReceiver{})
	if err != nil {
		fractallogger.Panicf(globalCancel, "Error initializing seelogBridge: %s", err)
	}

	// Below is (a heavily modified version of) the original main() body from the
	// ECS agent itself. Note that we modify `ecsapp.Run()` to take no arguments
	// instead of reading in `os.Args[1:]`.
	ecslogger.InitSeelog(seelogBridge)
	exitCode := ecsapp.Run([]string{})

	// TODO: pass in the context and cancel functions into the actual ecsapp.Run() command

	// If we got here, then that means that the ecsagent has exited for some reason.
	fractallogger.Panicf(globalCancel, "ECS Agent exited with code %d", exitCode)
}
