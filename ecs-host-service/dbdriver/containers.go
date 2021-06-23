package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"

	"github.com/jackc/pgtype"

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
func VerifyAllocatedContainer(userID fctypes.UserID) (fctypes.FractalID, error) {
	if !enabled {
		// We must simulate the webserver generating an ID for the container.
		return fctypes.FractalID(utils.RandHex(30)), nil
	}
	if dbpool == nil {
		return "", utils.MakeError("VerifyAllocatedContainer() called but dbdriver is not initialized!")
	}

	instanceName, err := aws.GetInstanceName()
	if err != nil {
		return "", utils.MakeError("Couldn't verify container for user %s: couldn't get instance name: %s", userID, err)
	}

	q := queries.NewQuerier(dbpool)
	rows, err := q.VerifyAllocatedContainer(context.Background(), string(instanceName), string(userID))
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

func WriteContainerStatus(containerID fctypes.FractalID, status ContainerStatus) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("WriteContainerStatus() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.WriteContainerStatus(context.Background(), pgtype.Varchar{
		String: string(status),
		Status: pgtype.Present,
	}, string(containerID))
	if err != nil {
		return utils.MakeError("Couldn't write status %s for container %s: error updating existing row in table `hardware.container_info`: %s", status, containerID, err)
	}
	logger.Infof("Updated status in database for container %s to %s: %s", containerID, status, result)

	return nil
}

// Check if container exists, etc.
func RemoveContainer(containerID fctypes.FractalID) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("RemoveContainer() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.RemoveContainer(context.Background(), string(containerID))
	if err != nil {
		return utils.MakeError("Couldn't remove container %s from database: %s", containerID, err)
	}
	logger.Infof("Removed row in database for container %s: %s", containerID, result)

	return nil
}
