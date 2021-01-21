package ecsagent

import (
	"math/rand"
	"os"
	"time"

	ecsapp "github.com/fractal/ecs-agent/agent/app"
	ecslogger "github.com/fractal/ecs-agent/agent/logger"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

func init() {
	rand.Seed(time.Now().UnixNano())

	// If we don't make this directory, the ecs-agent crashes.
	err := os.MkdirAll("/var/log/ecs/data/", 0755)
	if err != nil {
		logger.Panicf("Could not make directory /var/log/ecs/data: Error: %s", err)
	}
}

func ECSAgentMain() {
	ecslogger.InitSeelog()
	os.Exit(ecsapp.Run(os.Args[1:]))
}
