package dbdriver // import "github.com/whisthq/whist/backend/services/host-service/dbdriver"

import (
	"context"
	"math/rand"
	"time"

	"github.com/jackc/pgtype"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver/queries"
)

// As long as this channel is blocking, we should keep sending "alive"
// heartbeats. As soon as the channel is closed, we could optionally send a
// "dying" heartbeat, but we no longer send any more heartbeats.
var heartbeatKeepAlive = make(chan interface{}, 1)

func heartbeatGoroutine() {
	defer logger.Infof("Finished heartbeat goroutine.")
	timerChan := make(chan interface{})

	// Send initial heartbeat right away
	if err := writeHeartbeat(); err != nil {
		logger.Errorf("Error writing initial heartbeat: %s", err)
	}

	// Instead of running exactly every minute, we choose a random time in the
	// range [55, 65] seconds to prevent waves of hosts repeatedly crowding the
	// database.
	for {
		sleepTime := 60000 - rand.Intn(10001)
		timer := time.AfterFunc(time.Duration(sleepTime)*time.Millisecond, func() { timerChan <- struct{}{} })

		select {
		case <-heartbeatKeepAlive:
			// If we hit this case, that means that `heartbeatKeepAlive` was either
			// closed or written to (it should not be written to), but either way,
			// it's time to die.

			utils.StopAndDrainTimer(timer)
			return

		case <-timerChan:
			// There's just no time to die
			if err := writeHeartbeat(); err != nil {
				logger.Errorf("Error writing heartbeat: %s", err)
			}
		}
	}

}

// writeHeartbeat() is used to update the database with the latest metrics
// about this instance. This should only be called by the heartbeat goroutine,
// though technically Postgres atomicity should make this safe to call from
// concurrent goroutines.
func writeHeartbeat() error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("writeHeartbeat() called but dbdriver is not initialized!")
	}

	q := queries.NewQuerier(dbpool)

	instanceID, err := aws.GetInstanceID()
	if err != nil {
		return utils.MakeError("Couldn't write heartbeat: couldn't get instance name: %s", err)
	}

	result, err := q.WriteHeartbeat(context.Background(), pgtype.Timestamptz{
		Time:   time.Now(),
		Status: pgtype.Present,
	}, string(instanceID))
	if err != nil {
		return utils.MakeError("Couldn't write heartbeat: error updating existing row in table `whist.instances`: %s", err)
	} else if result.RowsAffected() == 0 {
		return utils.MakeError("Writing heartbeat updated zero rows in database! Instance row seems to be missing.")
	}

	return nil
}
