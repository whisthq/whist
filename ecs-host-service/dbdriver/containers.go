package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"

	"github.com/fractal/fractal/ecs-host-service/dbdriver/queries"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/fctypes"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata/aws"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// This file is concerned with database interactions at the container-level.

// A ContainerStatus represents a possible status that a container can have in the database.
type ContainerStatus string

// These represent the currently-defined statuses for containers.
const (
	ContainerStatusAllocated ContainerStatus = "ALLOCATED"
	ContainerStatusRunning   ContainerStatus = "RUNNING"
	ContainerStatusDying     ContainerStatus = "DYING"
)

// VerifyAllocatedContainer verifies that this host service is indeed expecting
// a container for the given user. On success, it returns the ID of the
// container.
func VerifyAllocatedContainer(ctx context.Context, userID fctypes.UserID) (fctypes.FractalID, error) {
	if !enabled {
		return "", nil
	}
	if dbpool == nil {
		return "", utils.MakeError("VerifyContainer() called but dbdriver is not initialized!")
	}

	instanceName, err := aws.GetInstanceName()
	if err != nil {
		return "", utils.MakeError("Couldn't verify container for user %s: couldn't get instance name: %s", userID, err)
	}

	q := queries.NewQuerier(dbpool)
	rows, err := q.VerifyAllocatedContainer(ctx, string(instanceName), string(userID))
	if err != nil {
		return "", utils.MakeError("Couldn't verify container for user %s: couldn't verify query: %s", userID, err)
	}

	// We expect that `rows` should contain exactly one, allocated (but not
	// running) container. Any other state is an error.
	if len(rows) == 0 {
		return "", utils.MakeError("Couldn't verify container for user %s: didn't find any container rows in the database for this user on this instance!", userID)
	} else if len(rows) > 1 {
		return "", utils.MakeError("Couldn't verify container for user %s: found too many container rows in the database for this user on this instance! Rows: %#v", userID, rows)
	} else if rows[0].Status.String != string(ContainerStatusAllocated) {
		return "", utils.MakeError(`Couldn't verify container for user %s: found a container row in the database for this instance, but it's in the wrong state. Expected "%s", got "%s".`, userID, ContainerStatusAllocated, rows[0].Status.String)
	}

	return fctypes.FractalID(rows[0].ContainerID.String), nil
}

func WriteContainerStatus(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("WriteContainerStatus() called but dbdriver is not initialized!")
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
