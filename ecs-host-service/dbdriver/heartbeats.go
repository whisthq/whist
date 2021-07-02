package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"math/rand"
	"time"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata/aws"
	"github.com/fractal/fractal/ecs-host-service/metrics"
	"github.com/fractal/fractal/ecs-host-service/utils"

	"github.com/fractal/fractal/ecs-host-service/dbdriver/queries"
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
		case _, _ = <-heartbeatKeepAlive:
			// If we hit this case, that means that `heartbeatKeepAlive` was either
			// closed or written to (it should not be written to), but either way,
			// it's time to die.

			// Stop timer to avoid leaking a goroutine (not that it matters if we're
			// shutting down, but still).
			if !timer.Stop() {
				<-timer.C
			}
			return

		case _ = <-timerChan:
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

	instanceName, err := aws.GetInstanceName()
	if err != nil {
		return utils.MakeError("Couldn't write heartbeat: couldn't get instance name: %s", err)
	}
	latestMetrics, errs := metrics.GetLatest()
	if len(errs) != 0 {
		return utils.MakeError("Couldn't write heartbeat: errors getting metrics: %+v", errs)
	}

	params := queries.WriteHeartbeatParams{
		MemoryRemainingKB:    int(latestMetrics.AvailableMemoryKB),
		NanoCPUsRemainingKB:  int(latestMetrics.NanoCPUsRemaining),
		GpuVramRemainingKb:   int(latestMetrics.FreeVideoMemoryKB),
		LastUpdatedUtcUnixMs: int(time.Now().UnixNano() / 1000),
		InstanceName:         string(instanceName),
	}
	result, err := q.WriteHeartbeat(context.Background(), params)
	if err != nil {
		return utils.MakeError("Couldn't write heartbeat: error updating existing row in table `hardware.instance_info`: %s", err)
	}
	logger.Infof("Wrote heartbeat %+v with result %s", params, result)

	// TODO: parse the command tag and error if the result is 0

	return nil
}
