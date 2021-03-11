package fractalcontainer

// This package, and its children, are meant to be low-level enough that it can
// be imported by both `https://github.com/fractal/ecs-agent` AND higher-level
// host service packages.

import (
	"sync"

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
	AllocatePortBindings([]PortBinding) ([]PortBinding, error)
	GetFractalID() FractalID
}

func New(fid FractalID) FractalContainer {
	return &containerData{FractalID: fid}
}

type containerData struct {
	rwlock sync.RWMutex
	FractalID
	DockerID
	appName        string
	userID         string
	uinputDevices  UinputDevices
	deviceMappings []dockercontainer.DeviceMapping
	PortBindings   []PortBinding
}

func (c *containerData) GetFractalID() FractalID {
	return c.FractalID
}
