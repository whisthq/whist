package ecsagent

import (
	"math/rand"
	"os"
	"time"

	ecsapp "github.com/fractal/ecs-agent/agent/app"
	ecslogger "github.com/fractal/ecs-agent/agent/logger"
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

func ECSAgentMain() {
	ecslogger.InitSeelog()
	os.Exit(ecsapp.Run(os.Args[1:]))
}
