package dbdriver // import "github.com/whisthq/whist/backend/services/host-service/dbdriver"

import (
	"context"
	"math/rand"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver/queries"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
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
	MandelboxStatusAllocated  MandelboxStatus = "ALLOCATED"
	MandelboxStatusConnecting MandelboxStatus = "CONNECTING"
	MandelboxStatusRunning    MandelboxStatus = "RUNNING"
	MandelboxStatusDying      MandelboxStatus = "DYING"
)

// VerifyAllocatedMandelbox verifies that this host service is indeed expecting
// the provided mandelbox for the given user, and if found marks it as
// connecting.
func VerifyAllocatedMandelbox(userID types.UserID, mandelboxID types.MandelboxID) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("VerifyAllocatedMandelbox() called but dbdriver is not initialized!")
	}

	// Create a transaction to register the instance, since we are querying and
	// writing separately.
	tx, err := dbpool.BeginTx(context.Background(), pgx.TxOptions{IsoLevel: pgx.ReadCommitted})
	if err != nil {
		return utils.MakeError("Couldn't register instance: unable to begin transaction: %s", err)
	}
	// Safe to do even if committed -- see tx.Rollback() docs.
	defer tx.Rollback(context.Background())

	instanceID, err := aws.GetInstanceID()
	if err != nil {
		return utils.MakeError("Couldn't verify mandelbox %s for user %s: couldn't get instance name: %s", mandelboxID, userID, err)
	}

	q := queries.NewQuerier(tx)
	rows, err := q.FindMandelboxByID(context.Background(), mandelboxID.String())
	if err != nil {
		return utils.MakeError("Couldn't verify mandelbox %s for user %s: error running query: %s", mandelboxID, userID, err)
	}

	// We expect that `rows` should contain exactly one, allocated (but not
	// running) mandelbox with a matching mandelbox ID. Any other state is an
	// error.
	if len(rows) == 0 {
		return utils.MakeError("Couldn't verify mandelbox %s for user %s: didn't find matching row in the database.", mandelboxID, userID)
	} else if rows[0].Status.String != string(MandelboxStatusAllocated) {
		return utils.MakeError(`Couldn't verify mandelbox %s for user %s: found a mandelbox row in the database for this instance, but it's in the wrong state. Expected "%s", got "%s".`, mandelboxID, userID, MandelboxStatusAllocated, rows[0].Status.String)
	} else if rows[0].InstanceID.String != string(instanceID) {
		return utils.MakeError(`Couldn't verify mandelbox %s for user %s: found an allocated mandelbox row in the database, but it has the wrong instanceName. Expected "%s", got "%s".`, mandelboxID, userID, rows[0].InstanceID.String, instanceID)
	}

	// Mark the container as connecting. We can't just use WriteMandelboxStatus
	// since we want to do it in a single transaction.
	result, err := q.WriteMandelboxStatus(context.Background(), pgtype.Varchar{
		String: string(MandelboxStatusConnecting),
		Status: pgtype.Present,
	}, mandelboxID.String())
	if err != nil {
		return utils.MakeError("Couldn't write status %s for mandelbox %s: error updating existing row in table `cloud.mandelbox_info`: %s", MandelboxStatusConnecting, mandelboxID, err)
	} else if result.RowsAffected() == 0 {
		return utils.MakeError("Couldn't write status %s for mandelbox %s: row in database missing!", MandelboxStatusConnecting, mandelboxID)
	}
	logger.Infof("Updated status in database for mandelbox %s to %s: %s", mandelboxID, MandelboxStatusConnecting, result)

	// Finish the transaction
	tx.Commit(context.Background())

	return nil
}

// WriteMandelboxStatus updates a mandelbox's status in the database.
func WriteMandelboxStatus(mandelboxID types.MandelboxID, status MandelboxStatus) error {
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
	}, mandelboxID.String())
	if err != nil {
		return utils.MakeError("Couldn't write status %s for mandelbox %s: error updating existing row in table `cloud.mandelbox_info`: %s", status, mandelboxID, err)
	} else if result.RowsAffected() == 0 {
		return utils.MakeError("Couldn't write status %s for mandelbox %s: row in database missing!", status, mandelboxID)
	}
	logger.Infof("Updated status in database for mandelbox %s to %s: %s", mandelboxID, status, result)

	return nil
}

// RemoveMandelbox removes a mandelbox's row in the database.
func RemoveMandelbox(mandelboxID types.MandelboxID) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("RemoveMandelbox() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.RemoveMandelbox(context.Background(), mandelboxID.String())
	if err != nil {
		return utils.MakeError("Couldn't remove mandelbox %s from database: %s", mandelboxID, err)
	} else if result.RowsAffected() == 0 {
		return utils.MakeError("Tried to remove mandelbox %s from database, but it was already gone!", mandelboxID)
	}
	logger.Infof("Removed row in database for mandelbox %s: %s", mandelboxID, result)

	return nil
}

// removeStaleMandelboxes removes mandelboxes that have an old creation time
// but are still marked as allocated, or have been marked connecting for too
// long.
func removeStaleMandelboxes(allocatedAge, connectingAge time.Duration) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("removeStaleMandelboxes() called but dbdriver is not initialized!")
	}

	instanceID, err := aws.GetInstanceID()
	if err != nil {
		return utils.MakeError("Couldn't remove stale allocated mandelboxes: %s", err)
	}

	logger.Infof("Arguments to stale routine are: %v, %v, %v", instanceID, MandelboxStatusAllocated, time.Now().Add(-1*allocatedAge))

	q := queries.NewQuerier(dbpool)
	result, err := q.RemoveStaleMandelboxes(context.Background(), queries.RemoveStaleMandelboxesParams{
		InstanceID:      string(instanceID),
		AllocatedStatus: string(MandelboxStatusAllocated),
		AllocatedCreationTimeThreshold: pgtype.Timestamptz{
			Time:   time.Now().Add(-1 * allocatedAge),
			Status: pgtype.Present,
		},
		ConnectingStatus: string(MandelboxStatusConnecting),
		ConnectingCreationTimeThreshold: pgtype.Timestamptz{
			Time:   time.Now().Add(-1 * connectingAge),
			Status: pgtype.Present,
		},
	})
	if err != nil {
		return utils.MakeError("Couldn't remove stale allocated mandelboxes from database: %s", err)
	}
	if result.RowsAffected() != 0 {
		// We avoid logging this every time to avoid polluting the logs.
		logger.Infof("Removed %v stale mandelboxes", result.RowsAffected())
		metrics.Add("CleanedStaleMandelboxes", result.RowsAffected())
	}
	return nil
}

func removeStaleMandelboxesGoroutine(globalCtx context.Context) {
	defer logger.Infof("Finished removeStaleMandelboxes goroutine.")
	timerChan := make(chan interface{})

	// Instead of running exactly every 10 seconds, we choose a random time in
	// the range [9.5, 10.5] seconds to prevent waves of hosts repeatedly crowding
	// the database.
	for {
		sleepTime := 10000 - rand.Intn(1001)
		timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- struct{}{} })

		select {
		case <-globalCtx.Done():
			// Remove allocated stale mandelboxes one last time
			if err := removeStaleMandelboxes(10*time.Second, 90*time.Second); err != nil {
				logger.Error(err)
			}

			utils.StopAndDrainTimer(timer)
			return

		case <-timerChan:
			if err := removeStaleMandelboxes(10*time.Second, 90*time.Second); err != nil {
				logger.Error(err)
			}
		}
	}
}
