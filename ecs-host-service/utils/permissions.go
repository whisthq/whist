package utils // import "github.com/fractal/fractal/ecs-host-service/utils"

import (
	"os"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// RequireRootPermissions checks that the program has been started with the
// correct permissions --- for now, we just want to run as root, but this
// service could be assigned its own user in the future.
func RequireRootPermissions() {
	if os.Geteuid() != 0 {
		logger.Panicf(nil, "This service needs to run as root!")
	}
}
