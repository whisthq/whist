package mandelbox // import "github.com/fractal/fractal/host-service/mandelbox"

// This package, and its children, are meant to be low-level enough that it can
// be imported by higher-level host service packages.

import (
	"context"
	"sync"

	"github.com/fractal/fractal/host-service/dbdriver"
	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/utils"

	"github.com/fractal/fractal/host-service/mandelbox/gpus"
	"github.com/fractal/fractal/host-service/mandelbox/portbindings"
	"github.com/fractal/fractal/host-service/mandelbox/ttys"
	"github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/mandelbox/uinputdevices"

	dockercontainer "github.com/docker/docker/api/types/container"
)

// Mandelbox represents a mandelbox as it is kept track of in this
// package. Higher layers of the host service use this interface.
type Mandelbox interface {
	GetMandelboxID() types.MandelboxID

	AssignToUser(types.UserID)
	GetUserID() types.UserID

	GetHostPort(mandelboxPort uint16, protocol portbindings.TransportProtocol) (uint16, error)
	GetIdentifyingHostPort() (uint16, error)

	InitializeTTY() error
	GetTTY() ttys.TTY

	AssignGPU() error
	GetGPU() gpus.Index

	// RegisterCreation is used to tell us the mapping between Docker IDs,
	// AppNames, and MandelboxIDs (which are used to track mandelboxes before they
	// are actually started, and therefore assigned a Docker runtime ID).
	// MandelboxIDs are also used to dynamically provide each mandelbox with a
	// directory that only that mandelbox has access to).
	RegisterCreation(types.DockerID, types.AppName) error
	GetDockerID() types.DockerID
	GetAppName() types.AppName

	GetConfigEncryptionToken() types.ConfigEncryptionToken
	SetConfigEncryptionToken(types.ConfigEncryptionToken)

	GetClientAppAccessToken() types.ClientAppAccessToken
	SetClientAppAccessToken(types.ClientAppAccessToken)

	// AssignPortBindings is used to request port bindings on the host for
	// mandelboxes. We allocate the host ports to be bound so the docker runtime
	// can actually bind them into the mandelbox.
	AssignPortBindings([]portbindings.PortBinding) error
	GetPortBindings() []portbindings.PortBinding

	GetDeviceMappings() []dockercontainer.DeviceMapping
	InitializeUinputDevices(*sync.WaitGroup) error

	// Writes files containing the TTY assignment and host port
	// corresponding to port 32262/tcp in the mandelbox, in a directory
	// accessible only to this mandelbox. These data are special because
	// they are computed and written when the mandelbox is created.
	WriteMandelboxParams() error
	// WriteProtocolTimeout writes a file containing the protocol timeout (i.e.
	// how long it will wait for a connection) to a directory accessible to only
	// this mandelbox.
	WriteProtocolTimeout(protocolTimeout int) error
	// MarkReady tells the protocol inside the mandelbox that it is ready to
	// start and accept connections.
	MarkReady() error

	// Populate the config folder under the mandelbox's MandelboxID for the
	// mandelbox's assigned user and running application.
	PopulateUserConfigs() error

	// GetContext provides the context corresponding to this specific mandelbox.
	GetContext() context.Context
	// Close cancels the mandelbox-specific context, triggering the cleanup of
	// all associated resources.
	Close()
}

// New creates a new Mandelbox given a parent context and a fractal ID.
func New(baseCtx context.Context, goroutineTracker *sync.WaitGroup, fid types.MandelboxID) Mandelbox {
	// We create a context for this mandelbox specifically.
	ctx, cancel := context.WithCancel(baseCtx)

	c := &mandelboxData{
		ctx:                  ctx,
		cancel:               cancel,
		mandelboxID:          fid,
		uinputDeviceMappings: []dockercontainer.DeviceMapping{},
		otherDeviceMappings:  []dockercontainer.DeviceMapping{},
	}

	c.createResourceMappingDir()

	trackMandelbox(c)

	// We start a goroutine that cleans up this Mandelbox as soon as its
	// context is cancelled. This ensures that we automatically clean up when
	// either `Close()` is called for this mandelbox, or the global context is
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-ctx.Done()

		// Mark mandelbox as dying in the database, but only if it's not a warmup
		if fid != "host-service-warmup" {
			if err := dbdriver.WriteMandelboxStatus(c.mandelboxID, dbdriver.MandelboxStatusDying); err != nil {
				logger.Error(err)
			}
		}

		untrackMandelbox(c)
		logger.Infof("Successfully untracked mandelbox %s", c.mandelboxID)

		c.rwlock.Lock()
		defer c.rwlock.Unlock()

		// Free port bindings
		portbindings.Free(c.portBindings)
		c.portBindings = nil
		logger.Infof("Successfully freed port bindings for mandelbox %s", c.mandelboxID)

		// Free uinput devices
		c.uinputDevices.Close()
		c.uinputDevices = nil
		c.uinputDeviceMappings = []dockercontainer.DeviceMapping{}
		logger.Infof("Successfully freed uinput devices for mandelbox %s", c.mandelboxID)

		// Free TTY
		ttys.Free(c.tty)
		logger.Infof("Successfully freed TTY %v for mandelbox %s", c.tty, c.mandelboxID)
		c.tty = 0

		if err := gpus.Free(c.gpuIndex); err != nil {
			logger.Errorf("Error freeing GPU %v for mandelbox %s: %s", c.gpuIndex, c.mandelboxID, err)
		} else {
			logger.Infof("Successfully freed GPU %v for mandelbox %s", c.gpuIndex, c.mandelboxID)
		}

		// Clean resource mappings
		c.cleanResourceMappingDir()
		logger.Infof("Successfully cleaned resource mapping dir for mandelbox %s", c.mandelboxID)

		// Backup and clean user config directory.
		err := c.backupUserConfigs()
		if err != nil {
			logger.Errorf("Error backing up user configs for MandelboxID %s. Error: %s", c.mandelboxID, err)
		} else {
			logger.Infof("Successfully backed up user configs for MandelboxID %s", c.mandelboxID)
		}
		c.cleanUserConfigDir()

		// Remove mandelbox from the database altogether, once again excluding warmups
		if fid != "host-service-warmup" {
			if err := dbdriver.RemoveMandelbox(c.mandelboxID); err != nil {
				logger.Error(err)
			}
		}

		logger.Infof("Cleaned up after Mandelbox %s", c.mandelboxID)
	}()

	return c
}

type mandelboxData struct {
	// TODO: re-evaluate whether we should still be doing this
	// Note that while the Go language authors discourage storing a context in a
	// struct, we need to do this to be able to call mandelbox methods from both
	// the host service and ecsagent (NOTE: we no longer use the ECS agent, this
	// can probably be reworked).
	ctx    context.Context
	cancel context.CancelFunc

	mandelboxID types.MandelboxID

	// We use rwlock to protect all the below fields.
	rwlock sync.RWMutex

	dockerID types.DockerID
	appName  types.AppName
	userID   types.UserID
	tty      ttys.TTY
	gpuIndex gpus.Index

	configEncryptionToken types.ConfigEncryptionToken
	clientAppAccessToken  types.ClientAppAccessToken

	uinputDevices        *uinputdevices.UinputDevices
	uinputDeviceMappings []dockercontainer.DeviceMapping
	// Not currently needed --- this is just here for extensibility
	otherDeviceMappings []dockercontainer.DeviceMapping

	portBindings []portbindings.PortBinding
}

// We do not lock here because the mandelboxID NEVER changes.
func (c *mandelboxData) GetMandelboxID() types.MandelboxID {
	return c.mandelboxID
}

func (c *mandelboxData) AssignToUser(u types.UserID) {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()
	c.userID = u
}

func (c *mandelboxData) GetUserID() types.UserID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.userID
}

func (c *mandelboxData) GetConfigEncryptionToken() types.ConfigEncryptionToken {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.configEncryptionToken
}

func (c *mandelboxData) SetConfigEncryptionToken(token types.ConfigEncryptionToken) {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	c.configEncryptionToken = token
}

func (c *mandelboxData) GetClientAppAccessToken() types.ClientAppAccessToken {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.clientAppAccessToken
}

func (c *mandelboxData) SetClientAppAccessToken(token types.ClientAppAccessToken) {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	c.clientAppAccessToken = token
}

func (c *mandelboxData) GetHostPort(mandelboxPort uint16, protocol portbindings.TransportProtocol) (uint16, error) {
	// Don't lock ourselves, since `c.GetPortBindings()` will lock for us.

	binds := c.GetPortBindings()
	for _, b := range binds {
		if b.Protocol == protocol && b.MandelboxPort == mandelboxPort {
			return b.HostPort, nil
		}
	}

	return 0, utils.MakeError("Couldn't GetHostPort(%v, %v) for mandelbox with MandelboxID %s", mandelboxPort, protocol, c.GetMandelboxID())
}

func (c *mandelboxData) GetIdentifyingHostPort() (uint16, error) {
	// Don't lock ourselves, since `c.GetHostPort()` will lock for us.
	return c.GetHostPort(32262, portbindings.TransportProtocolTCP)
}

func (c *mandelboxData) AssignGPU() error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	gpu, err := gpus.Allocate()
	if err != nil {
		return err
	}

	c.gpuIndex = gpu
	return nil
}

func (c *mandelboxData) GetGPU() gpus.Index {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()

	return c.gpuIndex
}

func (c *mandelboxData) InitializeTTY() error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	tty, err := ttys.Allocate()
	if err != nil {
		return err
	}

	c.tty = tty
	return nil
}

func (c *mandelboxData) GetTTY() ttys.TTY {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()

	return c.tty
}

func (c *mandelboxData) RegisterCreation(d types.DockerID, name types.AppName) error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	if len(d) == 0 || len(name) == 0 {
		return utils.MakeError("RegisterCreation: can't register mandelbox with an empty argument! mandelboxID: %s, dockerID: %s, name: %s", c.mandelboxID, d, name)
	}

	c.dockerID = d
	c.appName = name

	return nil
}

func (c *mandelboxData) GetDockerID() types.DockerID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.dockerID
}

func (c *mandelboxData) GetAppName() types.AppName {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.appName
}

func (c *mandelboxData) AssignPortBindings(desired []portbindings.PortBinding) error {
	result, err := portbindings.Allocate(desired)
	if err != nil {
		return err
	}

	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	c.portBindings = result
	return nil
}

func (c *mandelboxData) GetPortBindings() []portbindings.PortBinding {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.portBindings
}

func (c *mandelboxData) GetDeviceMappings() []dockercontainer.DeviceMapping {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return append(c.uinputDeviceMappings, c.otherDeviceMappings...)
}

func (c *mandelboxData) InitializeUinputDevices(goroutineTracker *sync.WaitGroup) error {
	devices, mappings, err := uinputdevices.Allocate()
	if err != nil {
		return utils.MakeError("Couldn't allocate uinput devices: %s", err)
	}

	c.rwlock.Lock()
	c.uinputDevices = devices
	c.uinputDeviceMappings = mappings
	defer c.rwlock.Unlock()

	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		err := uinputdevices.SendDeviceFDsOverSocket(c.ctx, goroutineTracker, devices, utils.TempDir+string(c.mandelboxID)+"/sockets/uinput.sock")
		if err != nil {
			logger.Errorf("SendDeviceFDsOverSocket returned for MandelboxID %s with error: %s", c.mandelboxID, err)
		} else {
			logger.Infof("SendDeviceFDsOverSocket returned successfully for MandelboxID %s", c.mandelboxID)
		}
	}()

	return nil
}

func (c *mandelboxData) GetContext() context.Context {
	return c.ctx
}

func (c *mandelboxData) Close() {
	// Cancel context, triggering the freeing up of all resources, including
	// tracked by goroutines (like cloud storage directories)
	c.cancel()
}
