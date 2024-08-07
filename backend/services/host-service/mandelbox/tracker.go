package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

import (
	"sync"
	"time"

	dockerclient "github.com/docker/docker/client"
	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// This file contains the code to track all Mandelboxes. We _need to_ do this
// because we need to be able to look up mandelboxes by `DockerID` in the
// top-level host-service package to handle deaths reported by Docker. We
// _can_ do this because Mandelboxes can only be created using New(), since the
// underlying datatype is not exported.

var tracker = make(map[types.MandelboxID]Mandelbox)
var trackerLock sync.RWMutex

func trackMandelbox(mandelbox Mandelbox) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	tracker[mandelbox.GetID()] = mandelbox
}

func untrackMandelbox(mandelbox Mandelbox) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	delete(tracker, mandelbox.GetID())
}

// LookUpByDockerID finds a mandelbox by its Docker ID. Note that this function
// (sequentially) gets a lock on every single mandelbox.
func LookUpByDockerID(DockerID types.DockerID) (Mandelbox, error) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, mandelbox := range tracker {
		d := mandelbox.GetDockerID()
		if d == DockerID {
			return mandelbox, nil
		}
	}
	return nil, utils.MakeError("none of the tracked mandelboxes has the docker id %s", DockerID)
}

// LookUpByMandelboxID finds a mandelbox by its Mandelbox ID.
// This function does not acquire locks on mandelboxes, only the tracker.
func LookUpByMandelboxID(mandelboxID types.MandelboxID) (Mandelbox, error) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	if _, ok := tracker[mandelboxID]; ok {
		return tracker[mandelboxID], nil
	}
	return nil, utils.MakeError("mandelbox %s is not tracked", mandelboxID)
}

// GetMandelboxCount gets the current number of waiting mandelboxes running on the instance.
func GetMandelboxCount() int32 {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	var currentWaitingMandelboxes []types.MandelboxID

	for _, m := range tracker {
		// Only consider waiting mandelboxes to which users haven't connected yet.
		if m.GetStatus() == dbdriver.MandelboxStatusWaiting {
			currentWaitingMandelboxes = append(currentWaitingMandelboxes, m.GetID())
		}
	}

	return int32(len(currentWaitingMandelboxes))
}

// StopWaitingMandelboxes will stop all mandelboxes to which users never
// connected. This function should only be called once the global context
// gets cancelled.
func StopWaitingMandelboxes(dockerClient dockerclient.CommonAPIClient) {
	logger.Infof("Cleaning up waiting mandelboxes that were never connected to.")
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, m := range tracker {
		if m.GetStatus() == dbdriver.MandelboxStatusWaiting {
			// Now cancel the mandelbox context
			m.Close()

			// Gracefully shut down the mandelbox Docker container
			stopTimeout := 60 * time.Second
			err := dockerClient.ContainerStop(m.GetContext(), string(m.GetDockerID()), &stopTimeout)

			if err != nil {
				logger.Warningf("Failed to gracefully stop mandelbox docker container.")
			}
		}
	}
}

// RemoveStaleMandelboxes cancels the context of mandelboxes that have an
// old creation time but are still marked as allocated, or have been marked
// connecting for too long.
func RemoveStaleMandelboxes(allocatedAgeLimit, connectingAgeLimit time.Duration) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, mandelbox := range tracker {
		timeSinceLastUpdate := time.Since(mandelbox.GetLastUpdatedTime())
		// If a mandelbox has the `ALLOCATED` or `CONNECTING` status, and the
		// last time since it was updated is greater than the allocated and
		// connecting limits, then it is considered stale and will be cleaned up.
		if mandelbox.GetStatus() == dbdriver.MandelboxStatusAllocated &&
			timeSinceLastUpdate > allocatedAgeLimit ||
			mandelbox.GetStatus() == dbdriver.MandelboxStatusConnecting &&
				timeSinceLastUpdate > connectingAgeLimit {
			logger.Infof("Cleaning up stale mandelbox %s", mandelbox.GetID())
			mandelbox.Close()
		}
	}
}
