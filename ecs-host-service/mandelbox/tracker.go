package mandelbox // import "github.com/fractal/fractal/ecs-host-service/mandelbox"

import (
	"sync"

	"github.com/fractal/fractal/ecs-host-service/mandelbox/types"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// This file contains the code to track all Mandelboxes. We _need to_ do this
// because we need to be able to look up mandelboxes by `DockerID` in the
// top-level ecs-host-service package to handle deaths reported by Docker. We
// _can_ do this because Mandelboxes can only be created using New(), since the
// underlying datatype is not exported.

var tracker = make(map[types.MandelboxID]Mandelbox)
var trackerLock sync.RWMutex

func trackMandelbox(fc Mandelbox) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	tracker[fc.GetMandelboxID()] = fc
}

func untrackMandelbox(fc Mandelbox) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	delete(tracker, fc.GetMandelboxID())
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
