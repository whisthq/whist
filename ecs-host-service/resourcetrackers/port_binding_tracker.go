package resourcetrackers

import (
	ecsagent "github.com/fractal/fractal/ecs-host-service/ecsagent"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	httpserver "github.com/fractal/fractal/ecs-host-service/httpserver"
)

const (
	MIN_ALLOWED_PORT = 1024
	MAX_ALLOWED_PORT = 49151 // Obtained from reading though Docker daemon source code

	FREE     = "free"
	RESERVED = "reserved"
)

// Unexported maps to keep track of free vs allocated ports
var tcpPorts map[uint16]string = make(map[uint16]string)
var udpPorts map[uint16]string = make(map[uint16]string)

// PortBindings is an exported map from containers' FractalIDs to their PortBindings
var PortBindings map[string][]ecsagent.PortBindings

func init() {
	// Mark certain ports as "reserved" so they don't get allocated for containers
	tcpPorts[httpserver.PortToListen] = RESERVED // For the host service itself
}
