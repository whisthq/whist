/*
Package mandelbox provides the abstractions of mandelboxes managed by Whist.
It defines the types and implements lots of functionality associated with
mandelboxes. In `tracker.go`, it also keeps a list of all mandelboxes on this
host at any given point.

This package, and its children, are meant to be low-level enough that it can
be imported by higher-level host service packages.
*/
package mandelbox // import "github.com/fractal/fractal/host-service/mandelbox"

import (
	"context"
	"strings"
	"sync"

	"github.com/aws/aws-sdk-go-v2/feature/s3/manager"
	"github.com/fractal/fractal/host-service/dbdriver"
	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/metadata"
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
	GetID() types.MandelboxID

	AssignToUser(types.UserID)
	GetUserID() types.UserID

	GetHostPort(mandelboxPort uint16, protocol portbindings.TransportProtocol) (uint16, error)
	GetIdentifyingHostPort() (uint16, error)

	InitializeTTY() error
	GetTTY() ttys.TTY

	AssignGPU() error
	GetGPU() gpus.Index

	// RegisterCreation is used to tell us the mapping between Docker IDs
	// and MandelboxIDs (which are used to track mandelboxes before they
	// are actually started, and therefore assigned a Docker runtime ID).
	// MandelboxIDs are also used to dynamically provide each mandelbox with a
	// directory that only that mandelbox has access to).
	RegisterCreation(types.DockerID) error
	SetAppName(types.AppName) error
	GetDockerID() types.DockerID
	GetAppName() types.AppName

	GetConfigEncryptionToken() types.ConfigEncryptionToken
	SetConfigEncryptionToken(types.ConfigEncryptionToken)

	GetJSONData() string
	SetJSONData(string)
	WriteJSONData() error

	// Decrypts the config encryption token and writes the user configs
	DecryptUserConfigs() error

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

	// Download the user configs from S3 and writes them to buffer
	DownloadUserConfigs() error

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

	mandelbox := &mandelboxData{
		ctx:                  ctx,
		cancel:               cancel,
		ID:                   fid,
		uinputDeviceMappings: []dockercontainer.DeviceMapping{},
		otherDeviceMappings:  []dockercontainer.DeviceMapping{},
	}

	mandelbox.createResourceMappingDir()

	trackMandelbox(mandelbox)

	// Mount /dev/fuse for in-mandelbox FUSE filesystems to work
	mandelbox.otherDeviceMappings = append(mandelbox.otherDeviceMappings, dockercontainer.DeviceMapping{
		PathOnHost:        "/dev/fuse",
		PathInContainer:   "/dev/fuse",
		CgroupPermissions: "rwm", // read, write, mknod (the default)
	})

	// We start a goroutine that cleans up this Mandelbox as soon as its
	// context is cancelled. This ensures that we automatically clean up when
	// either `Close()` is called for this mandelbox, or the global context is
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-ctx.Done()

		// Mark mandelbox as dying in the database, but only if it's not a warmup
		if fid != types.MandelboxID(utils.PlaceholderWarmupUUID()) {
			if err := dbdriver.WriteMandelboxStatus(mandelbox.ID, dbdriver.MandelboxStatusDying); err != nil {
				logger.Error(err)
			}
		}

		untrackMandelbox(mandelbox)
		logger.Infof("Successfully untracked mandelbox %s", mandelbox.ID)

		mandelbox.rwlock.Lock()
		defer mandelbox.rwlock.Unlock()

		// Free port bindings
		portbindings.Free(mandelbox.portBindings)
		mandelbox.portBindings = nil
		logger.Infof("Successfully freed port bindings for mandelbox %s", mandelbox.ID)

		// Free uinput devices
		mandelbox.uinputDevices.Close()
		mandelbox.uinputDevices = nil
		mandelbox.uinputDeviceMappings = []dockercontainer.DeviceMapping{}
		logger.Infof("Successfully freed uinput devices for mandelbox %s", mandelbox.ID)

		// Free TTY
		ttys.Free(mandelbox.tty)
		logger.Infof("Successfully freed TTY %v for mandelbox %s", mandelbox.tty, mandelbox.ID)
		mandelbox.tty = 0

		// CI does not have GPUs
		if !metadata.IsRunningInCI() {
			if err := gpus.Free(mandelbox.gpuIndex, mandelbox.ID); err != nil {
				logger.Errorf("Error freeing GPU %v for mandelbox %s: %s", mandelbox.gpuIndex, mandelbox.ID, err)
			} else {
				logger.Infof("Successfully freed GPU %v for mandelbox %s", mandelbox.gpuIndex, mandelbox.ID)
			}
		}

		// Clean resource mappings
		mandelbox.cleanResourceMappingDir()
		logger.Infof("Successfully cleaned resource mapping dir for mandelbox %s", mandelbox.ID)

		// Backup and clean user config directory.
		err := mandelbox.backupUserConfigs()
		if err != nil {
			logger.Errorf("Error backing up user configs for MandelboxID %s. Error: %s", mandelbox.ID, err)
		} else {
			logger.Infof("Successfully backed up user configs for MandelboxID %s", mandelbox.ID)
		}
		mandelbox.cleanUserConfigDir()

		// Remove mandelbox from the database altogether, once again excluding warmups
		if fid != types.MandelboxID(utils.PlaceholderWarmupUUID()) {
			if err := dbdriver.RemoveMandelbox(mandelbox.ID); err != nil {
				logger.Error(err)
			}
		}

		logger.Infof("Cleaned up after Mandelbox %s", mandelbox.ID)
	}()

	return mandelbox
}

type mandelboxData struct {
	// TODO: re-evaluate whether we should still be doing this
	// Note: while the Go language authors discourage storing a context in a
	// struct, we need to do this to be able to call mandelbox methods from both
	// the host service and ecsagent (NOTE: we no longer use the ECS agent, this
	// can probably be reworked).
	ctx    context.Context
	cancel context.CancelFunc

	ID types.MandelboxID

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
	// We use this to mount devices like /dev/fuse
	otherDeviceMappings []dockercontainer.DeviceMapping

	// We use this to apply any additional configs the user
	// might have (dark mode, location, etc.)
	JSONData string

	// We use these to download an decrypt the user configs
	// from s3.
	unpackedConfigDir string
	configBuffer      *manager.WriteAtBuffer

	portBindings []portbindings.PortBinding
}

// We do not lock here because the mandelboxID NEVER changes.
func (mandelbox *mandelboxData) GetID() types.MandelboxID {
	return mandelbox.ID
}

func (mandelbox *mandelboxData) AssignToUser(u types.UserID) {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()
	mandelbox.userID = u
}

func (mandelbox *mandelboxData) GetUserID() types.UserID {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.userID
}

func (mandelbox *mandelboxData) GetConfigEncryptionToken() types.ConfigEncryptionToken {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.configEncryptionToken
}

func (mandelbox *mandelboxData) SetConfigEncryptionToken(token types.ConfigEncryptionToken) {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()
	mandelbox.configEncryptionToken = token
}

func (mandelbox *mandelboxData) GetClientAppAccessToken() types.ClientAppAccessToken {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.clientAppAccessToken
}

func (mandelbox *mandelboxData) SetClientAppAccessToken(token types.ClientAppAccessToken) {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()
	mandelbox.clientAppAccessToken = token
}

func (mandelbox *mandelboxData) GetJSONData() string {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.JSONData
}

func (mandelbox *mandelboxData) SetJSONData(JSONData string) {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()

	mandelbox.JSONData = JSONData
}

func (mandelbox *mandelboxData) GetHostPort(mandelboxPort uint16, protocol portbindings.TransportProtocol) (uint16, error) {
	// Don't lock ourselves, since `c.GetPortBindings()` will lock for us.

	binds := mandelbox.GetPortBindings()
	for _, b := range binds {
		if b.Protocol == protocol && b.MandelboxPort == mandelboxPort {
			return b.HostPort, nil
		}
	}

	return 0, utils.MakeError("Couldn't GetHostPort(%v, %v) for mandelbox with MandelboxID %s", mandelboxPort, protocol, mandelbox.GetID())
}

func (mandelbox *mandelboxData) GetIdentifyingHostPort() (uint16, error) {
	// Don't lock ourselves, since `c.GetHostPort()` will lock for us.
	return mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
}

func (mandelbox *mandelboxData) AssignGPU() error {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	gpu, err := gpus.Allocate(mandelbox.ID)
	if err != nil {
		return err
	}

	mandelbox.gpuIndex = gpu
	return nil
}

func (mandelbox *mandelboxData) GetGPU() gpus.Index {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()

	return mandelbox.gpuIndex
}

func (mandelbox *mandelboxData) InitializeTTY() error {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	tty, err := ttys.Allocate()
	if err != nil {
		return err
	}

	mandelbox.tty = tty
	return nil
}

func (mandelbox *mandelboxData) GetTTY() ttys.TTY {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()

	return mandelbox.tty
}

func (mandelbox *mandelboxData) RegisterCreation(d types.DockerID) error {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	if len(d) == 0 {
		return utils.MakeError("RegisterCreation: can't register mandelbox %s with empty docker ID", mandelbox.ID)
	}

	mandelbox.dockerID = d
	return nil
}

func (mandelbox *mandelboxData) SetAppName(name types.AppName) error {
	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	if len(name) == 0 {
		return utils.MakeError("SetAppName: can't set mandelbox app name to empty for mandelboxID: %s", mandelbox.ID)
	}

	mandelbox.appName = name
	return nil
}

func (mandelbox *mandelboxData) GetDockerID() types.DockerID {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.dockerID
}

func (mandelbox *mandelboxData) GetAppName() types.AppName {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.appName
}

func (mandelbox *mandelboxData) AssignPortBindings(desired []portbindings.PortBinding) error {
	result, err := portbindings.Allocate(desired)
	if err != nil {
		return err
	}

	mandelbox.rwlock.Lock()
	defer mandelbox.rwlock.Unlock()

	mandelbox.portBindings = result
	return nil
}

func (mandelbox *mandelboxData) GetPortBindings() []portbindings.PortBinding {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return mandelbox.portBindings
}

func (mandelbox *mandelboxData) GetDeviceMappings() []dockercontainer.DeviceMapping {
	mandelbox.rwlock.RLock()
	defer mandelbox.rwlock.RUnlock()
	return append(mandelbox.uinputDeviceMappings, mandelbox.otherDeviceMappings...)
}

func (mandelbox *mandelboxData) InitializeUinputDevices(goroutineTracker *sync.WaitGroup) error {
	devices, mappings, err := uinputdevices.Allocate()
	if err != nil {
		return utils.MakeError("Couldn't allocate uinput devices: %s", err)
	}

	mandelbox.rwlock.Lock()
	mandelbox.uinputDevices = devices
	mandelbox.uinputDeviceMappings = mappings
	defer mandelbox.rwlock.Unlock()

	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		err := uinputdevices.SendDeviceFDsOverSocket(mandelbox.ctx, goroutineTracker, devices, utils.TempDir+mandelbox.ID.String()+"/sockets/uinput.sock")
		if err != nil {
			placeholderUUID := types.MandelboxID(utils.PlaceholderWarmupUUID())
			if mandelbox.ID == placeholderUUID && strings.Contains(err.Error(), "use of closed network connection") {
				logger.Warningf("SendDeviceFDsOverSocket returned for MandelboxID %s with error: %s", mandelbox.ID, err)
			} else {
				logger.Errorf("SendDeviceFDsOverSocket returned for MandelboxID %s with error: %s", mandelbox.ID, err)
			}
		} else {
			logger.Infof("SendDeviceFDsOverSocket returned successfully for MandelboxID %s", mandelbox.ID)
		}
	}()

	return nil
}

func (mandelbox *mandelboxData) GetContext() context.Context {
	return mandelbox.ctx
}

func (mandelbox *mandelboxData) Close() {
	// Cancel context, triggering the freeing up of all resources, including
	// tracked by goroutines (like cloud storage directories)
	mandelbox.cancel()
}
