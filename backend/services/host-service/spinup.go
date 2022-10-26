package main

import (
	"context"
	"crypto/sha1"
	"os"
	"path"
	"regexp"
	"strconv"
	"sync"
	"time"

	dockertypes "github.com/docker/docker/api/types"
	dockercontainer "github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/strslice"
	dockerclient "github.com/docker/docker/client"
	dockernat "github.com/docker/go-connections/nat"
	dockerunits "github.com/docker/go-units"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"
	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	mandelboxData "github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"go.uber.org/zap"
	"golang.org/x/sync/errgroup"
)

// StartMandelboxSpinUp will create and start a mandelbox, doing all the steps that can be done without the user's config token.
// Once the mandelbox is started, it effectively waits an infinite time until a user gets assigned to it and the remaining
// steps can continue.
func StartMandelboxSpinUp(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient, mandelboxID mandelboxtypes.MandelboxID, appName mandelboxtypes.AppName, mandelboxDieChan chan bool, kioskMode string, loadExtension string, localClient string) (mandelboxData.Mandelbox, error) {
	incrementErrorRate := func() {
		metrics.Increment("ErrorRate")
	}

	mandelbox := mandelboxData.New(context.Background(), goroutineTracker, mandelboxID, mandelboxDieChan)
	logger.Infof("SpinUpMandelbox(): created Mandelbox object %s", mandelbox.GetID())

	contextFields := []interface{}{
		zap.String("mandelbox_id", mandelbox.GetID().String()),
	}

	// If the creation of the mandelbox fails, we want to clean up after it. We
	// do this by setting `createFailed` to true until all steps are done, and
	// closing the mandelbox's context on function exit if `createFailed` is
	// still set to true.
	var createFailed bool = true
	defer func() {
		if createFailed {
			mandelbox.Close()
		}
	}()

	mandelbox.SetAppName(appName)

	// Generate a random hash to set as the server-side session id.
	// This session ID is used for the mandelbox services that start
	// before assigning a user.
	randBytes := make([]byte, 64)
	hash := sha1.New()
	hash.Write(randBytes)
	serverSessionID := utils.Sprintf("%x", hash.Sum(nil))

	// Do all startup tasks that can be done before Docker container creation in
	// parallel, stopping at the first error encountered
	preCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

	var hostPortForTCP32261, hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
	preCreateGroup.Go(func() error {
		if err := mandelbox.AssignPortBindings([]portbindings.PortBinding{
			{MandelboxPort: 32261, HostPort: 0, BindIP: "", Protocol: "tcp"},
			{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
			{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
			{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
		}); err != nil {
			return utils.MakeError("error assigning port bindings: %s", err)
		}
		logger.Infow(utils.Sprintf("SpinUpMandelbox(): successfully assigned port bindings %v", mandelbox.GetPortBindings()), contextFields)
		// Request port bindings for the mandelbox.
		var err32261, err32262, err32263, err32273 error
		hostPortForTCP32261, err32261 = mandelbox.GetHostPort(32261, portbindings.TransportProtocolTCP)
		hostPortForTCP32262, err32262 = mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
		hostPortForUDP32263, err32263 = mandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
		hostPortForTCP32273, err32273 = mandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
		if err32261 != nil || err32262 != nil || err32263 != nil || err32273 != nil {
			incrementErrorRate()
			return utils.MakeError("couldn't return host port bindings")
		}
		return nil
	})

	// Initialize Uinput devices for the mandelbox
	var devices []dockercontainer.DeviceMapping
	preCreateGroup.Go(func() error {
		if err := mandelbox.InitializeUinputDevices(goroutineTracker); err != nil {
			return utils.MakeError("error initializing uinput devices: %s", err)
		}

		logger.Infow("SpinUpMandelbox(): successfully initialized uinput devices.", contextFields)
		devices = mandelbox.GetDeviceMappings()
		return nil
	})

	// Allocate a TTY and GPU for the mandelbox
	preCreateGroup.Go(func() error {
		if err := mandelbox.InitializeTTY(); err != nil {
			return utils.MakeError("error initializing TTY: %s", err)
		}

		// CI does not have GPUs
		if !metadata.IsRunningInCI() && metadata.IsGPU() {
			if err := mandelbox.AssignGPU(); err != nil {
				return utils.MakeError("error assigning GPU: %s", err)
			}
		}

		logger.Infow(utils.Sprintf("SpinUpMandelbox(): successfully allocated TTY %v and assigned GPU %v", mandelbox.GetTTY(), mandelbox.GetGPU()), contextFields)
		return nil
	})

	err := preCreateGroup.Wait()
	if err != nil {
		incrementErrorRate()
		return mandelbox, err
	}

	// We need to compute the Docker image to use for this mandelbox. In local
	// development, we want to accept any string so that we can run arbitrary
	// containers as mandelboxes, including custom-tagged ones. However, in
	// deployments we only want to accept the specific apps that are enabled, and
	// don't want to leak the fact that we're using containers as the technology
	// for mandelboxes, so we construct the image string ourselves.
	var image string
	if metadata.IsLocalEnv() {

		regexes := []string{
			string(appName) + ":current-build",
			string(appName),
		}

		image = dockerImageFromRegexes(globalCtx, dockerClient, regexes)
	} else {
		image = utils.Sprintf("ghcr.io/whisthq/%s/%s:current-build", metadata.GetAppEnvironmentLowercase(), appName)
	}

	// We now create the underlying Docker container for this mandelbox.
	exposedPorts := make(dockernat.PortSet)
	exposedPorts[dockernat.Port("32261/tcp")] = struct{}{}
	exposedPorts[dockernat.Port("32262/tcp")] = struct{}{}
	exposedPorts[dockernat.Port("32263/udp")] = struct{}{}
	exposedPorts[dockernat.Port("32273/tcp")] = struct{}{}

	aesKey := utils.RandHex(16)
	mandelbox.SetPrivateKey(mandelboxtypes.PrivateKey(aesKey))

	// Common env variables
	envs := []string{
		utils.Sprintf("WHIST_AES_KEY=%s", aesKey),
		utils.Sprintf("SENTRY_ENV=%s", metadata.GetAppEnvironment()),
		utils.Sprintf("KIOSK_MODE=%t", kioskMode),
		utils.Sprintf("LOAD_EXTENSION=%t", loadExtension),
		utils.Sprintf("LOCAL_CLIENT=%t", localClient),
	}

	// Add additional env variables if host is using GPU
	if metadata.IsGPU() {
		logger.Infof("Using GPU env vars for container.")
		envs = append(envs, utils.Sprintf("NVIDIA_VISIBLE_DEVICES=%v", "all"))
		envs = append(envs, "NVIDIA_DRIVER_CAPABILITIES=all")
	}

	config := dockercontainer.Config{
		ExposedPorts: exposedPorts,
		Env:          envs,
		Image:        image,
		AttachStdin:  true,
		AttachStdout: true,
		AttachStderr: true,
		Tty:          true,
	}
	natPortBindings := make(dockernat.PortMap)
	natPortBindings[dockernat.Port("32261/tcp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForTCP32261)}}
	natPortBindings[dockernat.Port("32262/tcp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForTCP32262)}}
	natPortBindings[dockernat.Port("32263/udp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForUDP32263)}}
	natPortBindings[dockernat.Port("32273/tcp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForTCP32273)}}

	tmpfs := make(map[string]string)
	tmpfs["/run"] = "size=52428800"
	tmpfs["/run/lock"] = "size=52428800"

	hostConfig := dockercontainer.HostConfig{
		Binds: []string{
			"/sys/fs/cgroup:/sys/fs/cgroup:ro",
			path.Join(utils.WhistDir, mandelbox.GetID().String(), "mandelboxResourceMappings", ":", "/whist", "resourceMappings"),
			path.Join(utils.TempDir, mandelbox.GetID().String(), "sockets", ":", "tmp", "sockets"),
			path.Join(utils.TempDir, "logs", mandelbox.GetID().String(), serverSessionID, ":", "var", "log", "whist"), "/run/udev/data:/run/udev/data:ro",
		},
		PortBindings: natPortBindings,
		CapDrop:      strslice.StrSlice{"ALL"},
		CapAdd: strslice.StrSlice([]string{
			"SETPCAP",
			"MKNOD",
			"AUDIT_WRITE",
			"CHOWN",
			"NET_RAW",
			"DAC_OVERRIDE",
			"FOWNER",
			"FSETID",
			"KILL",
			"SETGID",
			"SETUID",
			"NET_BIND_SERVICE",
			"SYS_CHROOT",
			"SETFCAP",
			// NOTE THAT THE FOLLOWING ARE NOT ENABLED BY DEFAULT BY DOCKER --- THIS IS OUR DOING
			"SYS_NICE",
			"IPC_LOCK",
		}),
		ShmSize: 2147483648,
		Tmpfs:   tmpfs,
		Resources: dockercontainer.Resources{
			CPUShares: 2,
			Memory:    6552550944,
			NanoCPUs:  0,
			// Don't need to set CgroupParent, since each mandelbox is its own task.
			// We're not using anything like AWS services, where we'd want to put
			// several mandelboxes under one limit.
			Devices:            devices,
			KernelMemory:       0,
			KernelMemoryTCP:    0,
			MemoryReservation:  0,
			MemorySwap:         0,
			Ulimits:            []*dockerunits.Ulimit{},
			CPUCount:           0,
			CPUPercent:         0,
			IOMaximumIOps:      0,
			IOMaximumBandwidth: 0,
		},
		SecurityOpt: []string{
			"apparmor:mandelbox-apparmor-profile",
		},
	}
	mandelboxName := utils.Sprintf("%s-%s", appName, mandelboxID)
	re := regexp.MustCompile(`[^a-zA-Z0-9_.-]`)
	mandelboxName = re.ReplaceAllString(mandelboxName, "-")

	dockerBody, err := dockerClient.ContainerCreate(mandelbox.GetContext(), &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, mandelboxName)
	if err != nil {
		incrementErrorRate()
		return mandelbox, utils.MakeError("error running `create`: %s", err)
	}

	logger.Infow(utils.Sprintf("SpinUpMandelbox(): Value returned from ContainerCreate: %#v", dockerBody), contextFields)
	dockerID := mandelboxtypes.DockerID(dockerBody.ID)

	logger.Infow(utils.Sprintf("SpinUpMandelbox(): Successfully ran `create` command and got back runtime ID %s", dockerID), contextFields)

	postCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

	// Register docker ID in mandelbox object
	postCreateGroup.Go(func() error {
		err := mandelbox.RegisterCreation(dockerID)
		if err != nil {
			return utils.MakeError("error registering mandelbox creation with runtime ID %s: %s", dockerID, err)
		}

		logger.Infow(utils.Sprintf("SpinUpMandelbox(): Successfully registered mandelbox creation with runtime ID %s and AppName %s", dockerID, appName), contextFields)
		return nil
	})

	// Write mandelbox parameters to file
	postCreateGroup.Go(func() error {
		if err := mandelbox.WriteMandelboxParams(); err != nil {
			return utils.MakeError("error writing mandelbox params: %s", err)
		}

		// Server protocol waits 30 seconds for a client connection. However, in
		// localdev we default to an infinite timeout to enable easier testing.
		// In localdev environments, we can override this using an environment
		// variable.
		//
		// Since the server protocol only starts after the user configs are loaded,
		// zygote mandelboxes will wait forever until JSON transport. Once that occurs,
		// the server timeout will exit correctly if a client disconnects or fails to
		// connect.
		protocolTimeout := 30
		if metadata.IsLocalEnv() {
			localDevTimeout := os.Getenv("LOCALDEV_PROTOCOL_TIMEOUT")
			if localDevTimeout != "" {
				protocolTimeout, err = strconv.Atoi(localDevTimeout)
				if err != nil {
					logger.Warningf("SpinUpMandelbox(): Error parsing LOCALDEV_PROTOCOL_TIMEOUT envvar: %s", err)
				}
			} else {
				protocolTimeout = -1
			}
		}

		if err := mandelbox.WriteProtocolTimeout(protocolTimeout); err != nil {
			return utils.MakeError("error writing protocol timeout: %s", err)
		}

		logger.Infow("SpinUpMandelbox(): Successfully wrote parameters for mandelbox", contextFields)
		return nil
	})

	err = mandelbox.MarkParamsReady()
	if err != nil {
		incrementErrorRate()
		return mandelbox, utils.MakeError("error marking mandelbox as ready to start A/V + display: %s", err)
	}
	logger.Infow("SpinUpMandelbox(): Successfully marked mandelbox params as ready. A/V and display services can soon start.", contextFields)

	// Start Docker container
	postCreateGroup.Go(func() error {
		err = dockerClient.ContainerStart(mandelbox.GetContext(), string(dockerID), dockertypes.ContainerStartOptions{})
		if err != nil {
			return utils.MakeError("error starting mandelbox with runtime ID %s and MandelboxID %s: %s", dockerID, mandelboxID, err)
		}

		logger.Infow(utils.Sprintf("SpinUpMandelbox(): Successfully started mandelbox %s with ID %s", mandelboxName, mandelboxID), contextFields)
		return nil
	})

	// Mark as ready only when all startup tasks have completed
	err = postCreateGroup.Wait()
	if err != nil {
		incrementErrorRate()
		return mandelbox, err
	}

	// Mark mandelbox creation as successful, preventing cleanup on function
	// termination.
	createFailed = false
	mandelbox.SetStatus(dbdriver.MandelboxStatusWaiting)
	return mandelbox, nil
}

// FinishMandelboxSpinUp runs when a user gets assigned to a waiting mandelbox. This function does all the remaining steps in
// the spinup process that require a config token.
func FinishMandelboxSpinUp(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient, mandelboxSubscription subscriptions.Mandelbox, transportRequestMap map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest, transportMapLock *sync.Mutex, req *httputils.JSONTransportRequest) error {
	incrementErrorRate := func() {
		metrics.Increment("ErrorRate")
	}

	// Create a slice of the context information that will be passed to the logger.
	contextFields := []zap.Field{
		zap.String("instance_id", mandelboxSubscription.InstanceID),
		zap.String("mandelbox_id", mandelboxSubscription.ID.String()),
		zap.String("mandelbox_app", mandelboxSubscription.App),
		zap.String("session_id", mandelboxSubscription.SessionID),
		zap.String("user_id", string(mandelboxSubscription.UserID)),
		zap.Time("created_at", mandelboxSubscription.CreatedAt),
		zap.Time("updated_at", mandelboxSubscription.UpdatedAt),
	}

	// Make an interface{} slice for errors that get logged in this function.
	contextFieldSlice := make([]interface{}, len(contextFields))
	for i, v := range contextFields {
		contextFieldSlice[i] = v
	}

	logger.FastInfo("SpinUpMandelbox(): spinup started", contextFields...)

	// Then, verify that we are expecting this user to request a mandelbox.
	err := dbdriver.VerifyAllocatedMandelbox(mandelboxSubscription.UserID, mandelboxSubscription.ID)
	if err != nil {
		incrementErrorRate()
		return err
	}

	mandelbox, err := mandelboxData.LookUpByMandelboxID(mandelboxSubscription.ID)
	if err != nil {
		incrementErrorRate()
		return utils.MakeError("did not find existing mandelbox with ID %s", mandelboxSubscription.ID)
	}
	mandelbox.SetStatus(dbdriver.MandelboxStatusAllocated)

	// If the creation of the mandelbox fails, we want to clean up after it. We
	// do this by setting `createFailed` to true until all steps are done, and
	// closing the mandelbox's context on function exit if `createFailed` is
	// still set to true.
	var createFailed bool = true
	defer func() {
		if createFailed {
			mandelbox.Close()
		}
	}()

	mandelbox.AssignToUser(mandelboxSubscription.UserID)
	logger.FastInfo("SpinUpMandelbox(): Successfully assigned mandelbox", contextFields...)

	mandelbox.SetSessionID(mandelboxtypes.SessionID(mandelboxSubscription.SessionID))
	err = mandelbox.WriteSessionID()
	if err != nil {
		logger.Errorw(utils.Sprintf("Failed to write session id file: %s", err), contextFieldSlice)
	}

	logger.FastInfo(utils.Sprintf("SpinUpMandelbox(): Successfully wrote client session id file with content %s", mandelbox.GetSessionID()), contextFields...)

	// Request port bindings for the mandelbox.
	var (
		hostPortForTCP32261, hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
		err32261, err32262, err32263, err32273                                             error
	)
	hostPortForTCP32261, err32261 = mandelbox.GetHostPort(32261, portbindings.TransportProtocolTCP)
	hostPortForTCP32262, err32262 = mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
	hostPortForUDP32263, err32263 = mandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
	hostPortForTCP32273, err32273 = mandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
	if err32261 != nil || err32262 != nil || err32263 != nil || err32273 != nil {
		incrementErrorRate()
		return utils.MakeError("couldn't return host port bindings")
	}

	if req == nil {
		// Receive the JSON transport request from the client via the httpserver.
		jsonChan := getJSONTransportRequestChannel(mandelboxSubscription.ID, transportRequestMap, transportMapLock)

		// Set a timeout for the JSON transport request to prevent the mandelbox from waiting forever.
		select {
		case transportRequest := <-jsonChan:
			req = transportRequest
		case <-time.After(1 * time.Minute):
			// Clean up the mandelbox if the time out limit is reached
			return utils.MakeError("user failed to connect to mandelbox due to config token time out")
		}
	}
	mandelbox.SetStatus(dbdriver.MandelboxStatusConnecting)

	// While we wait for config decryption, write the config.json file with the
	// data received from JSON transport.
	err = mandelbox.WriteJSONData(req.JSONData)
	if err != nil {
		incrementErrorRate()
		return utils.MakeError("error writing config.json file for protocol: %s", err)
	}

	// Don't wait for whist-application to start up in local environment. We do
	// this because in local environments, we want to provide the developer a
	// shell into the container immediately, not conditional on everything
	// starting up properly.
	if !metadata.IsLocalEnv() {
		logger.FastInfo("SpinUpMandelbox(): Waiting for mandelbox whist-application to start up...", contextFields...)
		err = utils.WaitForFileCreation(path.Join(utils.WhistDir, mandelboxSubscription.ID.String(), "mandelboxResourceMappings"), "done_sleeping_until_X_clients", time.Second*20, nil)
		if err != nil {
			incrementErrorRate()
			return utils.MakeError("error waiting for mandelbox whist-application to start: %s", err)
		}

		logger.FastInfo("SpinUpMandelbox(): Finished waiting for mandelbox whist application to start up", contextFields...)
	}

	err = dbdriver.WriteMandelboxStatus(mandelboxSubscription.ID, dbdriver.MandelboxStatusRunning)
	if err != nil {
		incrementErrorRate()
		return utils.MakeError("error marking mandelbox running: %s", err)
	}
	logger.FastInfo("SpinUpMandelbox(): Successfully marked mandelbox as running", contextFields...)

	// Mark mandelbox creation as successful, preventing cleanup on function
	// termination.
	createFailed = false

	result := httputils.JSONTransportRequestResult{
		HostPortForTCP32261: hostPortForTCP32261,
		HostPortForTCP32262: hostPortForTCP32262,
		HostPortForUDP32263: hostPortForUDP32263,
		HostPortForTCP32273: hostPortForTCP32273,
		AesKey:              string(mandelbox.GetPrivateKey()),
	}
	req.ReturnResult(result, nil)
	mandelbox.SetStatus(dbdriver.MandelboxStatusRunning)
	logger.FastInfo("SpinUpMandelbox(): Finished starting up mandelbox", contextFields...)
	return nil
}
