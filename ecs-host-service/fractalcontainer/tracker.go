package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"sync"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// This file contains the code to track all FractalContainers. We _need to_ do
// this because we need to be able to look up containers by `FractalID` (and
// currently, `IdentifyingHostPort`, since the webserver is sending requests to the host
// service using `IdentifyingHostPort` instead of `FractalID`) in the top-level
// ecs-host-service package. We _can_ do this because FractalContainers can
// only be created using New(), since the underlying datatype is not exported.

// We optimize for the eventual case of needing to look up containers by
// `fractalID` _only_, not IdentifyingHostPort.
var tracker map[FractalID]FractalContainer = make(map[FractalID]FractalContainer)
var trackerLock sync.RWMutex

func trackContainer(fc FractalContainer) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	tracker[fc.GetFractalID()] = fc
}

func untrackContainer(fc FractalContainer) {
	trackerLock.Lock()
	defer trackerLock.Unlock()

	delete(tracker, fc.GetFractalID())
}

// LookUpByIdentifyingHostPort finds a container by the port 32262/tcp. Note
// that this function (sequentially) gets a lock on every single container.
func LookUpByIdentifyingHostPort(IdentifyingHostPort uint16) (FractalContainer, error) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, v := range tracker {
		p, _ := v.GetIdentifyingHostPort()
		if p == IdentifyingHostPort {
			return v, nil
		}
	}
	return nil, logger.MakeError("Couldn't find FractalContainer with IdentifyingHostPort %v", IdentifyingHostPort)
}

// LookUpByDockerID finds a container by its Docker ID. Note that this function
// (sequentially) gets a lock on every single container.
func LookUpByDockerID(DockerID DockerID) (FractalContainer, error) {
	trackerLock.RLock()
	defer trackerLock.RUnlock()

	for _, v := range tracker {
		d := v.GetDockerID()
		if d == DockerID {
			return v, nil
		}
	}
	return nil, logger.MakeError("Couldn't find FractalContainer with DockerID %s", DockerID)
}
