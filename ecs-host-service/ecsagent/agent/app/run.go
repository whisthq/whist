// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"). You may
// not use this file except in compliance with the License. A copy of the
// License is located at
//
//	http://aws.amazon.com/apache2.0/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.

package app

import (
	"context"
	"sync"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	log "github.com/cihub/seelog"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/app/args"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/logger"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/sighandlers/exitcodes"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/version"
)

// Run runs the ECS Agent App. It returns an exit code, which is used by
// main() to set the status code for the program
func Run(globalCtx context.Context, globalCancel context.CancelFunc, fractalGoroutineTracker *sync.WaitGroup) int {
	defer log.Flush()

	// Fractal-specific change: we removed the arg `arguments` and replaced it
	// with an empty slice.
	arguments := []string{}
	parsedArgs, err := args.New(arguments)
	if err != nil {
		return exitcodes.ExitTerminal
	}

	if *parsedArgs.License {
		return printLicense()
	} else if *parsedArgs.Version {
		return version.PrintVersion()
	} else if *parsedArgs.Healthcheck {
		// Timeout is purposely set to shorter than the default docker healthcheck
		// timeout of 30s. This is so that we can catch any http timeout and log the
		// issue within agent logs.
		// see https://docs.docker.com/engine/reference/builder/#healthcheck
		return runHealthcheck("http://localhost:51678/v1/metadata", time.Second*25)
	}

	if *parsedArgs.LogLevel != "" {
		logger.SetLevel(*parsedArgs.LogLevel, *parsedArgs.LogLevel)
	} else {
		logger.SetLevel(*parsedArgs.DriverLogLevel, *parsedArgs.InstanceLogLevel)
	}

	// Create an Agent object
	agent, err := newAgent(globalCtx, globalCancel, aws.BoolValue(parsedArgs.BlackholeEC2Metadata), parsedArgs.AcceptInsecureCert)
	if err != nil {
		// Failure to initialize either the docker client or the EC2 metadata
		// service client are non terminal errors as they could be transient
		return exitcodes.ExitError
	}

	switch {
	case *parsedArgs.ECSAttributes:
		// Print agent's ecs attributes based on its environment and exit
		return agent.printECSAttributes()
	case *parsedArgs.WindowsService:
		// Enable Windows Service
		return agent.startWindowsService()
	default:
		// Start the agent
		return agent.start(fractalGoroutineTracker)
	}
}
