package fractalcontainer

// This package, and its children, are meant to be low-level enough that it can
// be imported by both `https://github.com/fractal/ecs-agent` AND higher-level
// host service packages.

import (
	uinput "github.com/fractal/uinput-go"

	dockercontainer "github.com/docker/docker/api/types/container"
)

type FractalID string

type DockerID string

type CloudStorageDir string

type UinputDevices struct {
	absmouse uinput.TouchPad
	relmouse uinput.Mouse
	keyboard uinput.Keyboard
}

type FractalContainer interface {
	// AllocatePortBindings()
}

// func New(fid FractalID) FractalContainer {
// return &containerData{fid}
// }

type containerData struct {
	fractalID      FractalID
	dockerID       DockerID
	appName        string
	userID         string
	uinputDevices  UinputDevices
	deviceMappings []dockercontainer.DeviceMapping
	portBindings   []PortBinding
}
