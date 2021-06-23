package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// Check if container exists, etc.
func RegisterContainer(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("RegisterContainer() called but dbdriver is not initialized!")
	}

	logger.Errorf("Unimplemented")
	return nil
}

// Check if container exists, etc.
func RemoveContainer(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("RemoveContainer() called but dbdriver is not initialized!")
	}

	logger.Errorf("Unimplemented")
	return nil
}
