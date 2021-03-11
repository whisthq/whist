package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

// This package, and its children, are meant to be low-level enough that it can
// be imported by both `https://github.com/fractal/ecs-agent` AND higher-level
// host service packages.

import (
	"sync"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbinding"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/uinput"

	dockercontainer "github.com/docker/docker/api/types/container"
)

// We define special types for the following string types for all the benefits
// of type safety, including making sure we never switch Docker and Fractal
// IDs, for instance.

// A FractalID is a random hex string that the ecs agent creates for each container. We
// need some sort of identifier for each container, and we need it _before_
// Docker gives us back the runtime Docker ID for the container to coordinate
// communication between the host service and the ecs-agent. Currently,
// `FractalID` is also used as the replacement for `FRACTAL_ID_PLACEHOLDER` in
// the task definition for the mounted paths of this container on the host.
type FractalID string

// A DockerID is provided by Docker at container creation time.
type DockerID string

// AppName is defined as its own type so, in the future, we can always easily enforce
// that it is part of a limited set of values.
type AppName string

type UserID string

type CloudStorageDir string

type FractalContainer interface {
	GetFractalID() FractalID
	GetDockerID() DockerID
	GetAppName() AppName
	GetUserID() UserID

	GetPortBindings() []portbinding.PortBinding
	AssignPortBindings([]portbinding.PortBinding) error
	FreePortBindings()

	// AssignUinputDevices() ([]dockercontainer.DeviceMapping, error)
	// FreeUinputDevices()

	// Close()
}

func New(fid FractalID) FractalContainer {
	return &containerData{FractalID: fid}
}

type containerData struct {
	rwlock sync.RWMutex
	FractalID
	DockerID
	AppName
	UserID
	uinput.UinputDevices
	DeviceMappings []dockercontainer.DeviceMapping
	PortBindings   []portbinding.PortBinding
}

func (c *containerData) GetFractalID() FractalID {
	c.rwlock.RLock()
	c.rwlock.RUnlock()
	return c.FractalID
}

func (c *containerData) GetDockerID() DockerID {
	c.rwlock.RLock()
	c.rwlock.RUnlock()
	return c.DockerID
}

func (c *containerData) GetAppName() AppName {
	c.rwlock.RLock()
	c.rwlock.RUnlock()
	return c.AppName
}

func (c *containerData) GetUserID() UserID {
	c.rwlock.RLock()
	c.rwlock.RUnlock()
	return c.UserID
}

func (c *containerData) GetPortBindings() []portbinding.PortBinding {
	c.rwlock.RLock()
	c.rwlock.RUnlock()
	return c.PortBindings
}

func (c *containerData) AssignPortBindings(desired []portbinding.PortBinding) error {
	result, err := portbinding.Allocate(desired)
	if err != nil {
		return err
	}

	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	c.PortBindings = result
	return err
}

func (c *containerData) FreePortBindings() {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	portbinding.Free(c.PortBindings)
	c.PortBindings = nil
}
