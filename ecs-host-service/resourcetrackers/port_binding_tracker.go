package resourcetrackers

import (
	"math/rand"
	"time"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	fractaltypes "github.com/fractal/fractal/ecs-host-service/fractaltypes"
	httpserver "github.com/fractal/fractal/ecs-host-service/httpserver"
)

const (
	// Obtained from reading though Docker daemon source code
	MIN_ALLOWED_PORT = 1025  // inclusive
	MAX_ALLOWED_PORT = 49151 // exclusive

	RESERVED = "reserved"
)

// Unexported maps to keep track of free vs allocated host ports
type protocolSpecificMap map[uint16]string

var tcpPorts protocolSpecificMap = make(map[uint16]string)
var udpPorts protocolSpecificMap = make(map[uint16]string)

// PortBindings is an exported map from containers' FractalIDs to their PortBindings
var PortBindings map[string][]fractaltypes.PortBinding = make(map[string][]fractaltypes.PortBinding)

func init() {
	// Mark certain ports as "reserved" so they don't get allocated for containers
	tcpPorts[httpserver.PortToListen] = RESERVED // For the host service itself

	// Initialize random number generator
	rand.Seed(time.Now().UnixNano())
}

// AllocatePortBindings allocates the desired port bindings and updates `PortBindings` to match
func AllocatePortBindings(fractalID string, binds []fractaltypes.PortBinding) ([]fractaltypes.PortBinding, error) {
	var reterr error = nil

	// Check that PortBindings[fractalID] doesn't exist already, then set it to an empty slice
	if v, exists := PortBindings[fractalID]; exists {
		return nil, logger.MakeError("AllocatePorts: FractalID %v already has a PortBinding value: %v", fractalID, v)
	}
	PortBindings[fractalID] = []fractaltypes.PortBinding{}

	for _, v := range binds {
		reterr = allocateSinglePort(fractalID, v)
		if reterr != nil {
			break
		}
	}

	// If one allocation failed, we don't want to leak any ports.
	if reterr != nil {
		FreePortBindings(fractalID)
		return nil, reterr
	}

	return PortBindings[fractalID], nil
}

// assumes that PortBindings[`fractalID`] exists
func allocateSinglePort(fractalID string, bind fractaltypes.PortBinding) error {
	mapToUse, err := getMapFromProtocol(bind.Protocol)
	if err != nil {
		return logger.MakeError("allocateSinglePort: failed for FractalID %s. Error: %s", fractalID, err)
	}

	// If the given HostPort is nonzero, we want to use that one specifically. Else, we have to randomly allocate a free one.
	if bind.HostPort != 0 {
		// Check that this port isn't already allocated to a container
		if v, exists := (*mapToUse)[bind.HostPort]; exists {
			return logger.MakeError("allocateSinglePort: Could not allocate HostPort %v/%v for FractalID %v: Already bound to %v", bind.HostPort, bind.Protocol, fractalID, v)
		}

		// Mark it as allocated and update PortBindings
		(*mapToUse)[bind.HostPort] = fractalID
		PortBindings[fractalID] = append(PortBindings[fractalID], bind)

	} else {
		// Gotta allocate a port ourselves
		randomPort := randomPortInAllowedRange()
		numTries := 0
		for _, exists := (*mapToUse)[randomPort]; exists; randomPort = randomPortInAllowedRange() {
			numTries += 1
			if numTries >= 1000 {
				return logger.MakeError("Tried %v times to allocate a random host port for container port %v/%v for FractalID %v. Breaking out to avoid spinning for too long.", numTries, bind.HostPort, bind.Protocol, fractalID)
			}
		}

		// Mark it as allocated and update PortBindings
		(*mapToUse)[randomPort] = fractalID
		PortBindings[fractalID] = append(PortBindings[fractalID], fractaltypes.PortBinding{
			ContainerPort: bind.ContainerPort,
			HostPort:      randomPort,
			Protocol:      bind.Protocol,
		})
	}

	return nil
}

// FreePortBindings frees all ports that were mapped to the given FractalID
func FreePortBindings(fractalID string) {
	binds, exists := PortBindings[fractalID]
	if !exists {
		return
	}

	logger.Infof("Deleting all port bindings for FractalID %s", fractalID)

	for _, bind := range binds {
		mapToUse, err := getMapFromProtocol(bind.Protocol)
		if err != nil {
			logger.Errorf("FreePortBindings: failed for FractalID %s and bind %v. Error: %s", fractalID, bind, err)
			continue
		}

		delete(*mapToUse, bind.HostPort)
	}

	delete(PortBindings, fractalID)
}

// Helper function to generate random port values in the allowed range
func randomPortInAllowedRange() uint16 {
	return uint16(MIN_ALLOWED_PORT + rand.Intn(MAX_ALLOWED_PORT-MIN_ALLOWED_PORT))
}

// Helper function to check that a `fractaltypes.PortBinding` is valid (i.e.
// either "tcp" or "udp"), and return a pointer to the correct map to
// read/modify (i.e. either `tcpPorts` or `udpPorts`).
func getMapFromProtocol(protocol string) (*protocolSpecificMap, error) {
	switch protocol {
	case "tcp":
		return &tcpPorts, nil
	case "udp":
		return &udpPorts, nil
	default:
		return nil, logger.MakeError("getMapFromProtocol: received incorrect protocol: %v", protocol)
	}
}
