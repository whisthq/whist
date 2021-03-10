package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

import (
	"math/rand"
	"sync"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// A PortBinding represents a single port that is bound inside a container to a
// port with the same protocol on the host.
type PortBinding struct {
	// ContainerPort is the port inside the container
	ContainerPort uint16
	// HostPort is the port exposed on the host
	HostPort uint16
	// BindIP is the IP address to which the port is bound
	BindIP string `json:"BindIp"`
	// Protocol is the protocol of the port
	Protocol transportProtocol
}

const (
	// Obtained from reading though Docker daemon source code
	minAllowedPort = 1025  // inclusive
	maxAllowedPort = 49151 // exclusive

	reserved FractalID = FractalID("reserved")
)

// MarkPortReserved lets us mark certain ports as "reserved" so they don't get
// allocated for containers. This needs to be called at program initialization
// (ideally in an `init` function), before any containers are started.
func MarkPortReserved(num uint16, protocol transportProtocol) {
	mapToUse, err := getProtocolSpecificHostPortMap(protocol)
	if err != nil {
		logger.Panic(err)
	}
	(*mapToUse)[num] = reserved
	logger.Infof("Marked Port %v/%s as reserved", num, protocol)
}

// Unexported global maps to keep track of free vs allocated host ports
type protocolSpecificHostPortMap map[uint16]FractalID

var tcpPortMap protocolSpecificHostPortMap = make(map[uint16]FractalID)
var udpPortMap protocolSpecificHostPortMap = make(map[uint16]FractalID)

// Lock to protect `tcpPortMap` and `udpPortMap`
var portMapLocks = new(sync.Mutex)

// allocateSinglePort allocates a single port given a desired binding. It
// requires that `portMapLocks` is held throughout.
func allocateSinglePort(bind PortBinding, fid FractalID) (PortBinding, error) {
	mapToUse, err := getProtocolSpecificHostPortMap(bind.Protocol)
	if err != nil {
		return bind, logger.MakeError("allocateSinglePort: failed for FractalID %s. Error: %s", fid, err)
	}

	// If the given HostPort is nonzero, we want to use that one specifically. Else, we have to randomly allocate a free one.
	if bind.HostPort != 0 {
		// Check that this port isn't already allocated to a container
		if v, exists := (*mapToUse)[bind.HostPort]; exists {
			return bind, logger.MakeError("allocateSinglePort: Could not allocate HostPort %v/%v for FractalID %v: Already bound to %v", bind.HostPort, bind.Protocol, fid, v)
		}

		// Mark it as allocated and update PortBindings
		(*mapToUse)[bind.HostPort] = fid
		return bind, nil
	} else {
		// Gotta allocate a port ourselves
		randomPort := randomPortInAllowedRange()
		numTries := 0
		for _, exists := (*mapToUse)[randomPort]; exists; randomPort = randomPortInAllowedRange() {
			numTries++
			if numTries >= 100 {
				return bind, logger.MakeError("Tried %v times to allocate a random host port for container port %v/%v for FractalID %v. Breaking out to avoid spinning for too long.", numTries, bind.HostPort, bind.Protocol, fid)
			}
		}

		// Mark it as allocated and update PortBindings
		(*mapToUse)[randomPort] = fid
		return PortBinding{
			ContainerPort: bind.ContainerPort,
			HostPort:      randomPort,
			Protocol:      bind.Protocol,
			BindIP:        bind.BindIP,
		}, nil
	}
}

// freePortBindings marks all provided PortBindings as free in `tcpPortMap` and
// `udpPortMap`. It requires that `portMapLocks` is held throughout.
func freePortBindings(binds []PortBinding) {
	if binds == nil {
		return
	}

	logger.Infof("Deleting the following PortBindings: %v")

	for _, bind := range binds {
		mapToUse, err := getProtocolSpecificHostPortMap(bind.Protocol)
		if err != nil {
			logger.Errorf("FreePortBindings: failed for bind %v. Error: %s", bind, err)
		}

		delete(*mapToUse, bind.HostPort)
	}
}

// AllocatePortBindings allocates the desired port bindings and updates `PortBindings` to match. It returns the new value of `container.PortBindings`.
func (container *containerData) AllocatePortBindings(desiredBinds []PortBinding) ([]PortBinding, error) {
	// Lock the container for writing
	container.rwlock.Lock()
	defer container.rwlock.Unlock()

	portMapLocks.Lock()
	defer portMapLocks.Unlock()

	var reterr error = nil

	// Check that container.PortBindings doesn't already exist, then set it to an empty slice
	if len(container.PortBindings) != 0 {
		return nil, logger.MakeError("AllocatePortBindings: Container with FractalID %s already has nonempty PortBindings: %v", container.FractalID, container.PortBindings)
	}
	container.PortBindings = make([]PortBinding, len(desiredBinds))

	for _, v := range desiredBinds {
		var b PortBinding
		b, reterr = allocateSinglePort(v, container.FractalID)
		if reterr != nil {
			break
		} else {
			container.PortBindings = append(container.PortBindings, b)
		}
	}

	// If one allocation failed, we don't want to leak any ports.
	if reterr != nil {
		freePortBindings(container.PortBindings)
		container.PortBindings = nil
		return nil, reterr
	}

	return container.PortBindings, nil
}

// Helper function to generate random port values in the allowed range
func randomPortInAllowedRange() uint16 {
	return uint16(minAllowedPort + rand.Intn(maxAllowedPort-minAllowedPort))
}

// Helper function to check that a `fractaltypes.PortBinding` is valid (i.e.
// either "tcp" or "udp"), and return a pointer to the correct map to
// read/modify (i.e. either `tcpPorts` or `udpPorts`).
func getProtocolSpecificHostPortMap(protocol transportProtocol) (*protocolSpecificHostPortMap, error) {
	switch protocol {
	case TransportProtocolTCP:
		return &tcpPortMap, nil
	case TransportProtocolUDP:
		return &udpPortMap, nil
	default:
		return nil, logger.MakeError("getProtocolSpecificHostPortMap: received incorrect protocol: %v", protocol)
	}
}
