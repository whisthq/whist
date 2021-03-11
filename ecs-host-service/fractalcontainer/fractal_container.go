package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

// This package, and its children, are meant to be low-level enough that it can
// be imported by both `https://github.com/fractal/ecs-agent` AND higher-level
// host service packages.

import (
	"io"
	"sync"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/uinputdevices"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

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

	GetPortBindings() []portbindings.PortBinding
	AssignPortBindings([]portbindings.PortBinding) error
	FreePortBindings()

	GetDeviceMappings() []dockercontainer.DeviceMapping
	InitializeUinputDevices() error
	FreeUinputDevices()

	io.Closer
}

func New(fid FractalID) FractalContainer {
	return &containerData{
		fractalID:            fid,
		uinputDeviceMappings: []dockercontainer.DeviceMapping{},
		otherDeviceMappings:  []dockercontainer.DeviceMapping{},
	}
}

type containerData struct {
	rwlock    sync.RWMutex
	fractalID FractalID
	dockerID  DockerID
	appName   AppName
	userID    UserID

	uinputDevices        *uinputdevices.UinputDevices
	uinputDeviceMappings []dockercontainer.DeviceMapping
	// Not currently needed --- this is just here for extensibility
	otherDeviceMappings []dockercontainer.DeviceMapping

	portBindings []portbindings.PortBinding
}

func (c *containerData) GetFractalID() FractalID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.fractalID
}

func (c *containerData) GetDockerID() DockerID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.dockerID
}

func (c *containerData) GetAppName() AppName {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.appName
}

func (c *containerData) GetUserID() UserID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.userID
}

func (c *containerData) GetPortBindings() []portbindings.PortBinding {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.portBindings
}

func (c *containerData) AssignPortBindings(desired []portbindings.PortBinding) error {
	result, err := portbindings.Allocate(desired)
	if err != nil {
		return err
	}

	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	c.portBindings = result
	return err
}

func (c *containerData) FreePortBindings() {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	portbindings.Free(c.portBindings)
	c.portBindings = nil
}

func (c *containerData) GetDeviceMappings() []dockercontainer.DeviceMapping {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return append(c.uinputDeviceMappings, c.otherDeviceMappings...)
}

func (c *containerData) InitializeUinputDevices() error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	devices, mappings, err := uinputdevices.Allocate()
	if err != nil {
		return logger.MakeError("Couldn't allocate uinput devices: %s", err)
	}

	c.uinputDevices = devices
	c.uinputDeviceMappings = mappings

	go func() {
		err := uinputdevices.SendDeviceFDsOverSocket(devices, "/fractal/temp/"+string(c.fractalID)+"/sockets/uinput.sock")
		if err != nil {
			logger.Errorf("SendDeviceFDsOverSocket returned for FractalID %s with error: %s", c.fractalID, err)
		} else {
			logger.Infof("SendDeviceFDsOverSocket returned successfully for FractalID %s", c.fractalID)
		}
	}()

	return nil
}

func (c *containerData) FreeUinputDevices() {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()
	c.uinputDevices.Close()
	c.uinputDevices = nil
	c.uinputDeviceMappings = []dockercontainer.DeviceMapping{}
}

func (c *containerData) Close() error {
	// Each constituent function locks, so we don't need to lock here.
	c.FreePortBindings()
	c.FreeUinputDevices()

	return nil
}
