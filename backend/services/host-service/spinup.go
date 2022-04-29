package main

import (
	"context"
	"path"
	"regexp"
	"strings"
	"sync"
	"time"

	dockertypes "github.com/docker/docker/api/types"
	dockercontainer "github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/strslice"
	dockerclient "github.com/docker/docker/client"
	dockernat "github.com/docker/go-connections/nat"
	dockerunits "github.com/docker/go-units"
	"github.com/google/uuid"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"
	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox"
	mandelboxData "github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"golang.org/x/sync/errgroup"
)

// StartMandelboxSpinUp will create and start a mandelbox, doing all the steps that can be done without the user's config token.
// Once the mandelbox is started, it effectively waits an infinite time until a user gets assigned to it and the remaining
// steps can continue.
func StartMandelboxSpinUp(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient) mandelbox.Mandelbox {
	logAndReturnError := func(fmt string, v ...interface{}) {
		err := utils.MakeError("SpinUpMandelbox(): "+fmt, v...)
		logger.Error(err)
		metrics.Increment("ErrorRate")
	}

	mandelboxID := mandelboxtypes.MandelboxID(uuid.New())
	mandelbox := mandelboxData.New(context.Background(), goroutineTracker, mandelboxID)
	logger.Infof("SpinUpMandelbox(): created Mandelbox object %s", mandelbox.GetID())

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

	// replace chrome by brave here to test Brave -- note that the host-service is designed to only be able to launch one app across our whole backend
	var AppName mandelboxtypes.AppName = "chrome"
	mandelbox.SetAppName(AppName)

	// Do all startup tasks that can be done before Docker container creation in
	// parallel, stopping at the first error encountered
	preCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

	var hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
	preCreateGroup.Go(func() error {
		if err := mandelbox.AssignPortBindings([]portbindings.PortBinding{
			{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
			{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
			{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
		}); err != nil {
			return utils.MakeError("Error assigning port bindings: %s", err)
		}
		logger.Infof("SpinUpMandelbox(): successfully assigned port bindings %v", mandelbox.GetPortBindings())
		// Request port bindings for the mandelbox.
		var err32262, err32263, err32273 error
		hostPortForTCP32262, err32262 = mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
		hostPortForUDP32263, err32263 = mandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
		hostPortForTCP32273, err32273 = mandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
		if err32262 != nil || err32263 != nil || err32273 != nil {
			logAndReturnError("Couldn't return host port bindings.")
		}
		return nil
	})

	// Initialize Uinput devices for the mandelbox
	var devices []dockercontainer.DeviceMapping
	preCreateGroup.Go(func() error {
		if err := mandelbox.InitializeUinputDevices(goroutineTracker); err != nil {
			return utils.MakeError("Error initializing uinput devices: %s", err)
		}

		logger.Infof("SpinUpMandelbox(): successfully initialized uinput devices.")
		devices = mandelbox.GetDeviceMappings()
		return nil
	})

	// Allocate a TTY and GPU for the mandelbox
	preCreateGroup.Go(func() error {
		if err := mandelbox.InitializeTTY(); err != nil {
			return utils.MakeError("Error initializing TTY: %s", err)
		}

		// CI does not have GPUs
		if !metadata.IsRunningInCI() {
			if err := mandelbox.AssignGPU(); err != nil {
				return utils.MakeError("Error assigning GPU: %s", err)
			}
		}

		logger.Infof("SpinUpMandelbox(): successfully allocated TTY %v and assigned GPU %v for mandelbox %s", mandelbox.GetTTY(), mandelbox.GetGPU(), mandelboxID)
		return nil
	})

	err := preCreateGroup.Wait()
	if err != nil {
		logAndReturnError(err.Error())
		return nil
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
			string(AppName) + ":current-build",
			string(AppName),
		}

		image = dockerImageFromRegexes(globalCtx, dockerClient, regexes)
	} else {
		image = utils.Sprintf("ghcr.io/whisthq/%s/%s:current-build", metadata.GetAppEnvironmentLowercase(), AppName)
	}

	// We now create the underlying Docker container for this mandelbox.
	exposedPorts := make(dockernat.PortSet)
	exposedPorts[dockernat.Port("32262/tcp")] = struct{}{}
	exposedPorts[dockernat.Port("32263/udp")] = struct{}{}
	exposedPorts[dockernat.Port("32273/tcp")] = struct{}{}

	// TODO: also keep track of aes key
	aesKey := utils.RandHex(16)
	mandelbox.SetAESKey(mandelboxtypes.AESKey(aesKey))
	envs := []string{
		utils.Sprintf("WHIST_AES_KEY=%s", aesKey),
		utils.Sprintf("NVIDIA_VISIBLE_DEVICES=%v", "all"),
		"NVIDIA_DRIVER_CAPABILITIES=all",
		utils.Sprintf("SENTRY_ENV=%s", metadata.GetAppEnvironment()),
		utils.Sprintf("WHIST_INITIAL_USER_DATA_FILE=%v/%v", mandelboxData.UserInitialBrowserDirInMbox, mandelboxData.UserInitialBrowserFile),
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
	natPortBindings[dockernat.Port("32262/tcp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForTCP32262)}}
	natPortBindings[dockernat.Port("32263/udp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForUDP32263)}}
	natPortBindings[dockernat.Port("32273/tcp")] = []dockernat.PortBinding{{HostPort: utils.Sprintf("%v", hostPortForTCP32273)}}

	tmpfs := make(map[string]string)
	tmpfs["/run"] = "size=52428800"
	tmpfs["/run/lock"] = "size=52428800"

	// TODO: figure out how to bind the server logs without the session id.
	// Sanitize the received session ID. First, verify that the sessionID is a valid timestamp string.
	// If not, use a new timestamp.
	// _, err = strconv.ParseInt(mandelboxSubscription.SessionID, 10, 64)
	// if err != nil {
	// 	sessionID := strconv.FormatInt(time.Now().UnixMilli(), 10)
	// 	logger.Warningf("Malformed session ID: %v Using new session ID: %v", mandelboxSubscription.SessionID, sessionID)
	// 	mandelboxSubscription.SessionID = sessionID
	// }

	// mandelbox.SetSessionID(mandelboxtypes.SessionID(mandelboxSubscription.SessionID))

	hostConfig := dockercontainer.HostConfig{
		Binds: []string{
			"/sys/fs/cgroup:/sys/fs/cgroup:ro",
			path.Join(utils.WhistDir, mandelbox.GetID().String(), "mandelboxResourceMappings", ":", "/whist", "resourceMappings"),
			path.Join(utils.TempDir, mandelbox.GetID().String(), "sockets", ":", "tmp", "sockets"),
			// path.Join(utils.TempDir, "logs", mandelbox.GetID().String(), mandelboxSubscription.SessionID, ":", "var", "log", "whist"),
			"/run/udev/data:/run/udev/data:ro",
			path.Join(utils.WhistDir, mandelbox.GetID().String(), "userConfigs", "unpacked_configs", ":", "/whist", "userConfigs", ":rshared"),
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
	mandelboxName := utils.Sprintf("%s-%s", AppName, mandelboxID)
	re := regexp.MustCompile(`[^a-zA-Z0-9_.-]`)
	mandelboxName = re.ReplaceAllString(mandelboxName, "-")

	dockerBody, err := dockerClient.ContainerCreate(mandelbox.GetContext(), &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, mandelboxName)
	if err != nil {
		logAndReturnError("Error running `create` for %s:\n%s", mandelbox.GetID(), err)
		return nil
	}

	logger.Infof("SpinUpMandelbox(): Value returned from ContainerCreate: %#v", dockerBody)
	dockerID := mandelboxtypes.DockerID(dockerBody.ID)

	logger.Infof("SpinUpMandelbox(): Successfully ran `create` command and got back runtime ID %s", dockerID)

	postCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

	// Register docker ID in mandelbox object
	postCreateGroup.Go(func() error {
		err := mandelbox.RegisterCreation(dockerID)
		if err != nil {
			return utils.MakeError("Error registering mandelbox creation with runtime ID %s: %s", dockerID, err)
		}

		logger.Infof("SpinUpMandelbox(): Successfully registered mandelbox creation with runtime ID %s and AppName %s", dockerID, AppName)
		return nil
	})

	// Write mandelbox parameters to file
	postCreateGroup.Go(func() error {
		if err := mandelbox.WriteMandelboxParams(); err != nil {
			return utils.MakeError("Error writing mandelbox params: %s", err)
		}

		// Set the protocol timeout to an effectively infinite value, because the
		// mandelbox will be waiting for a user to get assigned to it.
		protocolTimeout := -1

		if err := mandelbox.WriteProtocolTimeout(protocolTimeout); err != nil {
			return utils.MakeError("Error writing protocol timeout: %s", err)
		}

		logger.Infof("SpinUpMandelbox(): Successfully wrote parameters for mandelbox %s", mandelboxID)
		return nil
	})

	// Start Docker container
	postCreateGroup.Go(func() error {
		err = dockerClient.ContainerStart(mandelbox.GetContext(), string(dockerID), dockertypes.ContainerStartOptions{})
		if err != nil {
			return utils.MakeError("Error starting mandelbox with runtime ID %s and MandelboxID %s: %s", dockerID, mandelboxID, err)
		}

		logger.Infof("SpinUpMandelbox(): Successfully started mandelbox %s with ID %s", mandelboxName, mandelboxID)
		return nil
	})

	// Mark as ready only when all startup tasks have completed
	err = postCreateGroup.Wait()
	if err != nil {
		logAndReturnError(err.Error())
		return nil
	}

	// Mark mandelbox creation as successful, preventing cleanup on function
	// termination.
	createFailed = false
	return mandelbox
}

// FinishMandelboxSpinUp runs when a user gets assigned to a waiting mandelbox. This function does all the remaining steps in
// the spinup process that require a config token.
func FinishMandelboxSpinUp(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient, sub *subscriptions.MandelboxEvent, transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest, transportMapLock *sync.Mutex) {

	logAndReturnError := func(fmt string, v ...interface{}) {
		err := utils.MakeError("SpinUpMandelbox(): "+fmt, v...)
		logger.Error(err)
		metrics.Increment("ErrorRate")
	}

	// mandelboxSubscription is the pubsub event received from Hasura.
	mandelboxSubscription := sub.Mandelboxes[0]
	req, _ := getAppName(mandelboxSubscription, transportRequestMap, transportMapLock)

	logger.Infof("SpinUpMandelbox(): spinup started for mandelbox %s", mandelboxSubscription.ID)

	// Then, verify that we are expecting this user to request a mandelbox.
	err := dbdriver.VerifyAllocatedMandelbox(mandelboxSubscription.UserID, mandelboxSubscription.ID)
	if err != nil {
		logAndReturnError("Unable to spin up mandelbox: %s", err)
		return
	}

	// TODO: figure out how to get the mandelbox object here
	mandelbox, err := mandelboxData.LookUpByMandelboxID(mandelboxSubscription.ID)
	if err != nil {
		logAndReturnError("Did not find existing mandelbox with ID %v", mandelboxSubscription.ID)
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

	mandelbox.AssignToUser(mandelboxSubscription.UserID)
	logger.Infof("SpinUpMandelbox(): Successfully assigned mandelbox %s to user %s", mandelboxSubscription.ID, mandelboxSubscription.UserID)

	// Request port bindings for the mandelbox.
	var (
		hostPortForTCP32262, hostPortForUDP32263, hostPortForTCP32273 uint16
		err32262, err32263, err32273                                  error
	)
	hostPortForTCP32262, err32262 = mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
	hostPortForUDP32263, err32263 = mandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
	hostPortForTCP32273, err32273 = mandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
	if err32262 != nil || err32263 != nil || err32273 != nil {
		logAndReturnError("Couldn't return host port bindings.")
	}

	// Begin loading user configs in parallel with the rest of the mandelbox startup procedure.
	sendEncryptionInfoChan, configDownloadErrChan := mandelbox.StartLoadingUserConfigs(globalCtx, globalCancel, goroutineTracker)
	defer close(sendEncryptionInfoChan)

	mandelbox.SetSessionID(mandelboxtypes.SessionID(mandelboxSubscription.SessionID))

	err = mandelbox.MarkParamsReady()
	if err != nil {
		logAndReturnError("Error marking mandelbox %s as ready to start A/V + display: %s", mandelboxSubscription.ID, err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully marked mandelbox %s params as ready. A/V and display services can soon start.", mandelboxSubscription.ID)

	logger.Infof("SpinUpMandelbox(): Waiting for config encryption token from client...")

	if req == nil {
		// Receive the JSON transport request from the client via the httpserver.
		jsonChan := getJSONTransportRequestChannel(mandelboxSubscription.ID, transportRequestMap, transportMapLock)

		// Set a timeout for the JSON transport request to prevent the mandelbox from waiting forever.
		select {
		case transportRequest := <-jsonChan:
			req = transportRequest
			mandelbox.SetConnectedStatus(true)
		case <-time.After(1 * time.Minute):
			// Clean up the mandelbox if the time out limit is reached
			logAndReturnError("User %v failed to connect to mandelbox %v due to config token time out. Not performing user-specific cleanup.", mandelbox.GetUserID(), mandelbox.GetID())
			mandelbox.SetConnectedStatus(false)
			return
		}
	}

	// Report the config encryption info to the config loader after making sure
	// it passes some basic sanity checks.
	sendEncryptionInfoChan <- mandelboxData.ConfigEncryptionInfo{
		Token:                          req.ConfigEncryptionToken,
		IsNewTokenAccordingToClientApp: req.IsNewConfigToken,
	}
	// We don't close the channel here, since we defer the close when we first
	// make it.

	// While we wait for config decryption, write the config.json file with the
	// data received from JSON transport.
	err = mandelbox.WriteJSONData(req.JSONData)
	if err != nil {
		logAndReturnError("Error writing config.json file for protocol in mandelbox %s for user %s: %s", mandelbox.GetID(), mandelbox.GetUserID(), err)
		return
	}

	// Wait for configs to be fully decrypted before we write any user initial
	// browser data.
	for err := range configDownloadErrChan {
		// We don't want these user config errors to be fatal, so we log them as
		// errors and move on.
		logger.Error(err)
	}

	// Read any existing imported extensions
	savedExtensions := mandelbox.GetSavedExtensions()

	// If the new request contains additional imported extensions, add them to the existing list
	if len(req.Extensions) > 0 {
		savedExtensions = configutils.UpdateImportedExtensions(savedExtensions, req.Extensions)
		if err = mandelbox.WriteSavedExtensions(savedExtensions); err != nil {
			logger.Errorf("Error writing imported extensions for mandelbox %s: %s", mandelbox.GetID(), err)
		}
	}

	// Unmarshal bookmarks into proper format
	var importedBookmarks configutils.Bookmarks
	if len(req.BookmarksJSON) > 0 {
		importedBookmarks, err = configutils.UnmarshalBookmarks(req.BookmarksJSON)
		if err != nil {
			// Bookmark import errors are not fatal
			logger.Errorf("Error unmarshalling bookmarks for mandelbox %s: %s", mandelbox.GetID(), err)
		}
	}

	// Write the user's initial browser data
	logger.Infof("SpinUpMandelbox(): Beginning storing user initial browser data for mandelbox %s", mandelboxSubscription.ID)
	err = mandelbox.WriteUserInitialBrowserData(mandelboxData.BrowserData{
		CookiesJSON:  req.CookiesJSON,
		Bookmarks:    &importedBookmarks,
		Extensions:   mandelboxtypes.Extensions(strings.Join(savedExtensions, ",")),
		Preferences:  req.Preferences,
		LocalStorage: req.LocalStorageJSON,
	})
	if err != nil {
		logger.Errorf("Error writing initial browser data for user %s for mandelbox %s: %s", mandelbox.GetUserID(), mandelboxSubscription.ID, err)
	} else {
		logger.Infof("SpinUpMandelbox(): Successfully wrote user initial browser data for mandelbox %s", mandelboxSubscription.ID)
	}

	// Unblock whist-startup.sh to start symlink loaded user configs
	err = mandelbox.MarkConfigReady()
	if err != nil {
		logAndReturnError("Error marking mandelbox %s configs as ready: %s", mandelboxSubscription.ID, err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully marked mandelbox %s as ready", mandelboxSubscription.ID)

	// Don't wait for whist-application to start up in local environment. We do
	// this because in local environments, we want to provide the developer a
	// shell into the container immediately, not conditional on everything
	// starting up properly.
	if !metadata.IsLocalEnv() {
		logger.Infof("SpinUpMandelbox(): Waiting for mandelbox %s whist-application to start up...", mandelboxSubscription.ID)
		err = utils.WaitForFileCreation(path.Join(utils.WhistDir, mandelboxSubscription.ID.String(), "mandelboxResourceMappings"), "done_sleeping_until_X_clients", time.Second*20, nil)
		if err != nil {
			logAndReturnError("Error waiting for mandelbox %s whist-application to start: %s", mandelboxSubscription.ID, err)
			return
		}

		logger.Infof("SpinUpMandelbox(): Finished waiting for mandelbox %s whist application to start up", mandelboxSubscription.ID)
	}

	err = dbdriver.WriteMandelboxStatus(mandelboxSubscription.ID, dbdriver.MandelboxStatusRunning)
	if err != nil {
		logAndReturnError("Error marking mandelbox running: %s", err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully marked mandelbox %s as running", mandelboxSubscription.ID)

	// Mark mandelbox creation as successful, preventing cleanup on function
	// termination.
	createFailed = false

	result := JSONTransportRequestResult{
		HostPortForTCP32262: hostPortForTCP32262,
		HostPortForUDP32263: hostPortForUDP32263,
		HostPortForTCP32273: hostPortForTCP32273,
		AesKey:              string(mandelbox.GetAESKey()),
	}
	req.ReturnResult(result, nil)
	logger.Infof("SpinUpMandelbox(): Finished starting up mandelbox %s", mandelbox.GetID())
}
