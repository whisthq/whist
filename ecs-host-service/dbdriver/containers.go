package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// VerifyContainer verifies that this host service is indeed expecting a
// container for the given user.
func VerifyContainer(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("VerifyContainer() called but dbdriver is not initialized!")
	}

	logger.Errorf("Unimplemented")
	return nil
}

func MarkContainerRunning(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("MarkContainerRunning() called but dbdriver is not initialized!")
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
