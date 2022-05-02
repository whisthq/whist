package mandelbox // import "github.com/whisthq/whist/backend/services/host-service/mandelbox"

import (
	"sync"

	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
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

	for _, v := range tracker {
		d := v.GetDockerID()
		if d == DockerID {
			return v, nil
		}
	}
	return nil, utils.MakeError("Couldn't find Mandelbox with DockerID %s", DockerID)
}

// LookUpByMandelboxID finds a mandelbox by its Mandelbox ID.
// This function does not acquire locks on mandelboxes, only the tracker.
func LookUpByMandelboxID(mandelboxID types.MandelboxID) (Mandelbox, error) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	if _, ok := tracker[mandelboxID]; ok {
		return tracker[mandelboxID], nil
	}
	return nil, utils.MakeError("Couldn't find Mandelbox with MandelboxID %s", mandelboxID)
}

// StopWaitingMandelboxes will stop all mandelboxes to which users never
// connected. This function should only be called once the global context
// gets cancelled.
func StopWaitingMandelboxes() {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, v := range tracker {
		if !v.GetConnectedStatus() {
			v.Close()
		}
	}
}
