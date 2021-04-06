package fractalcontainer // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer"

// This package, and its children, are meant to be low-level enough that it can
// be imported by both `https://github.com/fractal/ecs-agent` AND higher-level
// host service packages.

import (
	"context"
	"sync"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/ttys"
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

// UserID is defined as its own type as well so the compiler can check argument
// orders, etc. more effectively.
type UserID string

type ConfigEncryptionToken string
type ClientAppAccessToken string
type SetupEndpoint string

// FractalContainer represents a container as it is kept track of in this
// package. Both the ECS agent and higher layers of the host service use this
// interface.
type FractalContainer interface {
	GetFractalID() FractalID

	AssignToUser(UserID)
	GetUserID() UserID

	GetIdentifyingHostPort() (uint16, error)

	InitializeTTY() error
	GetTTY() ttys.TTY

	// RegisterCreation is used by the ecs-agent (built into the host service, in
	// package `ecsagent`) to tell us the mapping between Docker container IDs,
	// AppNames, and FractalIDs (which are used to track containers before they
	// are actually started, and therefore assigned a Docker runtime ID).
	// FractalIDs are also used to dynamically provide each container with a
	// directory that only that container has access to).
	RegisterCreation(DockerID, AppName) error
	GetDockerID() DockerID
	GetAppName() AppName

	GetConfigEncryptionToken() ConfigEncryptionToken
	SetConfigEncryptionToken(ConfigEncryptionToken)

	GetClientAppAccessToken() ClientAppAccessToken
	SetClientAppAccessToken(ClientAppAccessToken)

	// AssignPortBindings is used by the ecs-agent (built into the host service,
	// in package `ecsagent`) to request port bindings on the host for
	// containers. We allocate the host ports to be bound so the docker runtime
	// can actually bind them into the container.
	AssignPortBindings([]portbindings.PortBinding) error
	GetPortBindings() []portbindings.PortBinding

	GetDeviceMappings() []dockercontainer.DeviceMapping
	InitializeUinputDevices(*sync.WaitGroup) error

	// Writes files containing the TTY assignment and host port corresponding to
	// port 32262/tcp in the container, in a directory accessible only to this
	// container. These data are special because they are computed and written
	// when the container is created.
	WriteResourcesForProtocol() error
	// WriteStartValues() writes files containing the DPI, ContainerARN, and
	// UserID assigned to a directory accessible to only this container. These
	// data are only known once a container is assigned to a user and are
	// provided by the fractal webserver.
	WriteStartValues(dpi int, containerARN string) error
	// MarkReady tells the protocol inside the container that it is ready to
	// start and accept connections.
	MarkReady() error

	// Populate the config folder under the container's FractalID for the
	// container's assigned user and running application.
	PopulateUserConfigs() error

	// If the user ID is not set for the given fractalID yet, that means that both necessary tasks have not
	// been completed yet before setting the container as ready: these tasks are
	// handleSetConfigEncryptionTokenRequest and handleStartValuesRequest. If both tasks have completed,
	// then get the user's config and set the container as ready.
	CompleteContainerSetup(userID UserID, clientAppAccessToken ClientAppAccessToken, callerFunction SetupEndpoint) error

	AddCloudStorage(goroutineTracker *sync.WaitGroup, Provider cloudstorage.Provider, AccessToken string, RefreshToken string, Expiry string, TokenType string, ClientID string, ClientSecret string) error

	Close()
}

// New creates a new FractalContainer given a parent context and a fractal ID.
func New(baseCtx context.Context, goroutineTracker *sync.WaitGroup, fid FractalID) FractalContainer {
	// We create a context for this container specifically.
	ctx, cancel := context.WithCancel(baseCtx)

	c := &containerData{
		ctx:                  ctx,
		cancel:               cancel,
		fractalID:            fid,
		uinputDeviceMappings: []dockercontainer.DeviceMapping{},
		otherDeviceMappings:  []dockercontainer.DeviceMapping{},
	}

	c.createResourceMappingDir()

	trackContainer(c)

	// We start a goroutine that cleans up this FractalContainer as soon as its
	// context is cancelled. This ensures that we automatically clean up when
	// either `Close()` is called for this container, or the global context is
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-ctx.Done()

		untrackContainer(c)
		logger.Infof("Successfully untracked container %s", c.fractalID)

		c.rwlock.Lock()
		defer c.rwlock.Unlock()

		// Free port bindings
		portbindings.Free(c.portBindings)
		c.portBindings = nil
		logger.Infof("Successfully freed port bindings for container %s", c.fractalID)

		// Free uinput devices
		c.uinputDevices.Close()
		c.uinputDevices = nil
		c.uinputDeviceMappings = []dockercontainer.DeviceMapping{}
		logger.Infof("Successfully freed uinput devices for container %s", c.fractalID)

		// Free TTY
		ttys.Free(c.tty)
		logger.Infof("Successfully freed TTY %v for container %s", c.tty, c.fractalID)
		c.tty = 0

		// Clean resource mappings
		c.cleanResourceMappingDir()
		logger.Infof("Successfully cleaned resource mapping dir for container %s", c.fractalID)

		// Backup and clean user config directory.
		err := c.backupUserConfigs()
		if err != nil {
			logger.Errorf("Error backing up user configs for FractalID %s. Error: %s", c.fractalID, err)
		} else {
			logger.Infof("Successfully backed up user configs for FractalID %s", c.fractalID)
		}
		c.cleanUserConfigDir()

		logger.Infof("Cleaned up after FractalContainer %s", c.fractalID)
	}()

	return c
}

type containerData struct {
	// Note that while the Go language authors discourage storing a context in a
	// struct, we need to do this to be able to call container methods from both
	// the host service and ecsagent.
	ctx    context.Context
	cancel context.CancelFunc

	fractalID FractalID

	// We use rwlock to protect all the below fields.
	rwlock sync.RWMutex

	dockerID DockerID
	appName  AppName
	userID   UserID
	tty      ttys.TTY

	configEncryptionToken    ConfigEncryptionToken
	clientAppAccessToken     ClientAppAccessToken
	firstCalledSetupEndpoint SetupEndpoint

	uinputDevices        *uinputdevices.UinputDevices
	uinputDeviceMappings []dockercontainer.DeviceMapping
	// Not currently needed --- this is just here for extensibility
	otherDeviceMappings []dockercontainer.DeviceMapping

	portBindings []portbindings.PortBinding
}

// We do not lock here because the fractalID NEVER changes.
func (c *containerData) GetFractalID() FractalID {
	return c.fractalID
}

func (c *containerData) AssignToUser(u UserID) {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()
	c.userID = u
}

func (c *containerData) GetUserID() UserID {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.userID
}

func (c *containerData) GetConfigEncryptionToken() ConfigEncryptionToken {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.configEncryptionToken
}

func (c *containerData) SetConfigEncryptionToken(token ConfigEncryptionToken) {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	c.configEncryptionToken = token
}

func (c *containerData) GetClientAppAccessToken() ClientAppAccessToken {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.clientAppAccessToken
}

func (c *containerData) SetClientAppAccessToken(token ClientAppAccessToken) {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	c.clientAppAccessToken = token
}

func (c *containerData) GetIdentifyingHostPort() (uint16, error) {
	// Don't lock ourselves, since `c.GetPortBindings()` will lock for us.

	binds := c.GetPortBindings()
	for _, b := range binds {
		if b.Protocol == portbindings.TransportProtocolTCP && b.ContainerPort == 32262 {
			return b.HostPort, nil
		}
	}

	return 0, logger.MakeError("Couldn't getIdentifyingHostPort() for container with FractalID %s", c.GetFractalID())
}

func (c *containerData) InitializeTTY() error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	tty, err := ttys.Allocate()
	if err != nil {
		return err
	}

	c.tty = tty
	return nil
}

func (c *containerData) GetTTY() ttys.TTY {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()

	return c.tty
}

func (c *containerData) RegisterCreation(d DockerID, name AppName) error {
	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	if len(d) == 0 || len(name) == 0 {
		return logger.MakeError("RegisterCreatedContainer: can't register container with an empty argument! fractalID: %s, dockerID: %s, name: %s", c.fractalID, d, name)
	}

	c.dockerID = d
	c.appName = name

	return nil
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

func (c *containerData) AssignPortBindings(desired []portbindings.PortBinding) error {
	result, err := portbindings.Allocate(desired)
	if err != nil {
		return err
	}

	c.rwlock.Lock()
	defer c.rwlock.Unlock()

	c.portBindings = result
	return nil
}

func (c *containerData) GetPortBindings() []portbindings.PortBinding {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return c.portBindings
}

func (c *containerData) GetDeviceMappings() []dockercontainer.DeviceMapping {
	c.rwlock.RLock()
	defer c.rwlock.RUnlock()
	return append(c.uinputDeviceMappings, c.otherDeviceMappings...)
}

func (c *containerData) InitializeUinputDevices(goroutineTracker *sync.WaitGroup) error {
	devices, mappings, err := uinputdevices.Allocate()
	if err != nil {
		return logger.MakeError("Couldn't allocate uinput devices: %s", err)
	}

	c.rwlock.Lock()
	c.uinputDevices = devices
	c.uinputDeviceMappings = mappings
	defer c.rwlock.Unlock()

	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		err := uinputdevices.SendDeviceFDsOverSocket(c.ctx, goroutineTracker, devices, logger.TempDir+string(c.fractalID)+"/sockets/uinput.sock")
		if err != nil {
			logger.Errorf("SendDeviceFDsOverSocket returned for FractalID %s with error: %s", c.fractalID, err)
		} else {
			logger.Infof("SendDeviceFDsOverSocket returned successfully for FractalID %s", c.fractalID)
		}
	}()

	return nil
}

func (c *containerData) CompleteContainerSetup(userID UserID, clientAppAccessToken ClientAppAccessToken, callerFunction SetupEndpoint) error {
	// If the user ID has already been set, then this function has already been successfully called twice
	//    and shouldn't be run again
	// TODO: use a different synchronization mechanism
	if len(c.GetUserID()) > 0 {
		return nil
	}

	c.rwlock.Lock()

	// If a client app access token hasn't been set that means that this function
	//    is being called for the first time and initial values need to be set.
	if len(c.clientAppAccessToken) == 0 {
		c.clientAppAccessToken = clientAppAccessToken
		c.firstCalledSetupEndpoint = SetupEndpoint(callerFunction)

		c.rwlock.Unlock()
		return nil
	}

	// If a malicious user is requesting the same endpoint multiple times, they won't get past
	//     the first call because a different endpoint needs to be called to proceed, and there
	//     are only two endpoints that can lead to this point.
	// NOTE: if more endpoints are added to call `completeContainerSetup`, then please view the
	//    note above the declaration of `calledSetupEndpoints`.
	if c.firstCalledSetupEndpoint == callerFunction {
		c.rwlock.Unlock()
		return logger.MakeError("Same container setup endpoint called multiple times for user %s", userID)
	}

	c.rwlock.Unlock()

	c.AssignToUser(userID)

	var err error
	// Populate the user config folder for the container's app ONLY if both the client app
	//     and webserver have passed the same client app access token to the host service
	if clientAppAccessToken == c.GetClientAppAccessToken() {
		err = c.PopulateUserConfigs()
		if err != nil {
			logger.Error(err)
		}
	} else {
		// If the access tokens are not the same, then make sure the stored encryption token is empty.
		//     This will allow the container to still run, but prevent any app config from saving
		//     at the end of the session.
		logger.Errorf("Client app access tokens for user %s did not match - not retrieving config", userID)
		c.SetConfigEncryptionToken("")
	}

	// When both endpoints have been properly called, then mark the container as ready
	err = c.MarkReady()
	if err != nil {
		return err
	}

	return nil
}

func (c *containerData) Close() {
	// Cancel context, triggering the freeing up of all resources, including
	// tracked by goroutines (like cloud storage directories)
	c.cancel()
}
