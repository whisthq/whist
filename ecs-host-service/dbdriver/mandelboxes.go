package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"math/rand"
	"time"

	"github.com/jackc/pgtype"

	"github.com/fractal/fractal/ecs-host-service/dbdriver/queries"
	"github.com/fractal/fractal/ecs-host-service/mandelbox/types"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata/aws"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// This file is concerned with database interactions at the mandelbox-level.
// Note that we don't explicitly use transactions for any database interactions
// in this file, since the instance's singular host service should be the only
// agent interacting with the database rows for the mandelboxes once the
// webserver creates them in the "ALLOCATED" state.

// A MandelboxStatus represents a possible status that a mandelbox can have in the database.
type MandelboxStatus string

// These represent the currently-defined statuses for mandelboxes.
const (
	MandelboxStatusAllocated MandelboxStatus = "ALLOCATED"
	MandelboxStatusRunning   MandelboxStatus = "RUNNING"
	MandelboxStatusDying     MandelboxStatus = "DYING"
)

// VerifyAllocatedMandelbox verifies that this host service is indeed expecting
// the provided mandelbox for the given user.
func VerifyAllocatedMandelbox(userID types.UserID, mandelboxID types.FractalID) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("VerifyAllocatedMandelbox() called but dbdriver is not initialized!")
	}

	instanceName, err := aws.GetInstanceName()
	if err != nil {
		return utils.MakeError("Couldn't verify mandelbox for user %s: couldn't get instance name: %s", userID, err)
	}

	q := queries.NewQuerier(dbpool)
	rows, err := q.FindMandelboxesForUser(context.Background(), string(instanceName), string(userID))
	if err != nil {
		return utils.MakeError("Couldn't verify mandelbox for user %s: couldn't verify query: %s", userID, err)
	}

	// We expect that `rows` should contain exactly one, allocated (but not
	// running) mandelbox with a matching mandelbox ID. Any other state is an
	// error.
	if len(rows) == 0 {
		return utils.MakeError("Couldn't verify mandelbox for user %s: didn't find any mandelbox rows in the database for this user on this instance!", userID)
	} else if len(rows) > 1 {
		return utils.MakeError("Couldn't verify mandelbox for user %s: found too many mandelbox rows in the database for this user on this instance! Rows: %#v", userID, rows)
	} else if rows[0].Status.String != string(MandelboxStatusAllocated) {
		return utils.MakeError(`Couldn't verify mandelbox for user %s: found a mandelbox row in the database for this instance, but it's in the wrong state. Expected "%s", got "%s".`, userID, MandelboxStatusAllocated, rows[0].Status.String)
	} else if rows[0].MandelboxID.String != string(mandelboxID) {
		return utils.MakeError(`Couldn't verify mandelbox for user %s: found an allocated mandelbox row in the database, but it has the wrong fractalID. Expected "%s", got "%s".`, userID, rows[0].MandelboxID.String, mandelboxID)
	}

	return nil
}

// WriteMandelboxStatus updates a mandelbox's status in the database.
func WriteMandelboxStatus(mandelboxID types.FractalID, status MandelboxStatus) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("WriteMandelboxStatus() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.WriteMandelboxStatus(context.Background(), pgtype.Varchar{
		String: string(status),
		Status: pgtype.Present,
	}, string(mandelboxID))
	if err != nil {
		return utils.MakeError("Couldn't write status %s for mandelbox %s: error updating existing row in table `hardware.mandelbox_info`: %s", status, mandelboxID, err)
	}
	logger.Infof("Updated status in database for mandelbox %s to %s: %s", mandelboxID, status, result)

	return nil
}

// RemoveMandelbox removes a mandelbox's row in the database.
func RemoveMandelbox(mandelboxID types.FractalID) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("RemoveMandelbox() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.RemoveMandelbox(context.Background(), string(mandelboxID))
	if err != nil {
		return utils.MakeError("Couldn't remove mandelbox %s from database: %s", mandelboxID, err)
	}
	logger.Infof("Removed row in database for mandelbox %s: %s", mandelboxID, result)

	return nil
}

// removeStaleAllocatedMandelboxes removes mandelboxes that have an old creation
// time but are still marked as allocated.
func removeStaleAllocatedMandelboxes(age time.Duration) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("removeStaleAllocatedMandelboxes() called but dbdriver is not initialized!")
	}

	instanceName, err := aws.GetInstanceName()
	if err != nil {
		return utils.MakeError("Couldn't remove stale allocated mandelboxes: %s", err)
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.RemoveStaleAllocatedMandelboxes(context.Background(), queries.RemoveStaleAllocatedMandelboxesParams{
		InstanceName:          string(instanceName),
		Status:                string(MandelboxStatusAllocated),
		CreationTimeThreshold: int(time.Now().Add(-1*age).UnixNano() / 1000),
	})
	if err != nil {
		return utils.MakeError("Couldn't remove stale allocated mandelboxes from database: %s", err)
	}
	logger.Infof("Removed any stale allocated mandelboxes with result: %s", result)
	return nil
}

func removeStaleAllocatedMandelboxesGoroutine(globalCtx context.Context) {
	defer logger.Infof("Finished removeStaleAllocatedMandelboxes goroutine.")
	timerChan := make(chan interface{})

	// Instead of running exactly every 90 seconds, we choose a random time in
	// the range [95, 105] seconds to prevent waves of hosts repeatedly crowding
	// the database.
	for {
		sleepTime := 100000 - rand.Intn(10001)
		timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- struct{}{} })

		select {
		case <-globalCtx.Done():
			// Remove allocated stale mandelboxes one last time
			if err := removeStaleAllocatedMandelboxes(90 * time.Second); err != nil {
				logger.Error(err)
			}

			// Stop timer to avoid leaking a goroutine (not that it matters if we're
			// shutting down, but still).
			if !timer.Stop() {
				<-timer.C
			}
			return

		case _ = <-timerChan:
			if err := removeStaleAllocatedMandelboxes(90 * time.Second); err != nil {
				logger.Error(err)
			}
		}
	}
}
