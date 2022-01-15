/*
The Whist Host Service is responsible for orchestrating mandelboxes (i.e.
Whist-enabled containers) on EC2 instances (referred to as "hosts" throughout
the codebase). The host service is responsible for making Docker calls to start
and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on
the same host (by dynamically allocating and assigning resources), and for
passing startup data to the mandelboxes, both from the rest of the backend and
from the user's client application.

If you are just interested in seeing what endpoints the host service exposes
(i.e. for client-app development), check out the file `httpserver.go`.

The main package of the host service contains the main logic and the most
comments to explain the design decisions of the host service. It also contains
an HTTPS server that exposes the necessary endpoints and sets up the necessary
infrastructure for concurrent handlers, etc.
*/
package main

import (
	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via Sentry. Instead, use the
	// whistlogger package imported below as `logger`.

	"context"
	_ "embed"
	"io"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"golang.org/x/sync/errgroup"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry. For the same reason, we make sure not
	// to import the fmt package either, instead separating required
	// functionality in this imported package as well.

	logger "github.com/whisthq/whist/backend/services/whistlogger"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	mandelboxData "github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"

	dockertypes "github.com/docker/docker/api/types"
	dockercontainer "github.com/docker/docker/api/types/container"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/api/types/strslice"
	dockerclient "github.com/docker/docker/client"
	dockernat "github.com/docker/go-connections/nat"
	dockerunits "github.com/docker/go-units"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"
)

var shutdownInstanceOnExit bool = !metadata.IsLocalEnv()

func init() {
	// Initialize random number generator for all subpackages
	rand.Seed(time.Now().UnixNano())

	// Check that the program has been started with the correct permissions ---
	// for now, we just want to run as root, but this service could be assigned
	// its own user in the future.
	if os.Geteuid() != 0 {
		// We can do a "real" panic here because it's in an init function, so we
		// haven't even entered the host service main() yet.
		logger.Panicf(nil, "This service needs to run as root!")
	}
}

// createDockerClient creates a docker client. It returns an error if creation
// failed.
func createDockerClient() (*dockerclient.Client, error) {
	client, err := dockerclient.NewClientWithOpts(dockerclient.WithAPIVersionNegotiation())
	if err != nil {
		return nil, utils.MakeError("Error creating new Docker client: %s", err)
	}
	return client, nil
}

// Given a list of regexes, find a docker image whose name matches the earliest
// possible regex in the list.
func dockerImageFromRegexes(globalCtx context.Context, dockerClient dockerclient.CommonAPIClient, regexes []string) string {
	imageFilters := dockerfilters.NewArgs(
		dockerfilters.KeyValuePair{Key: "dangling", Value: "false"},
	)
	images, err := dockerClient.ImageList(globalCtx, dockertypes.ImageListOptions{All: false, Filters: imageFilters})
	if err != nil {
		return ""
	}

	for _, regex := range regexes {
		rgx := regexp.MustCompile(regex)
		for _, img := range images {
			for _, tag := range img.RepoTags {
				if rgx.MatchString(tag) {
					return tag
				}
			}
		}
	}
	return ""
}

// "Warm up" Docker. This is necessary because for some reason the first
// ContainerStart call by the host service is taking over a minute. Note that
// this doesn't seem to be happening on personal instances, only
// dev/staging/prod ones.
// TODO: figure out the root cause of this
// Note that we also need to "warm up" Chrome --- for some reason Chrome in the
// first container on a host is taking almost two minutes to start up and
// register itself as an X client at the moment. Therefore, we need to write
// the resources for the protocol in this function too.
// TODO: figure out the root cause of this too
// I'd really like to get to the root causes of these issues to avoid the code
// duplication between this function and SpinUpMandelbox. It doesn't make sense
// to pollute the code in that function with a flag for warming up, so we have
// to live with this duplication until we can find and solve the root cause.
func warmUpDockerClient(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, client dockerclient.CommonAPIClient) error {
	// We've gotta find a suitable image to use for warming up the client. We
	// prefer images that are most alike to those the host service will actually
	// be launching, so we use a list of regexes. We prefer matches for earlier
	// regexes in the list.

	// Note: the following code is theoretically inefficient but practically good
	// enough, since our data is small and this function is only run once.

	// Note that the first image in this list appears on local machines but not
	// dev/staging/prod ones, so we can use the same regex list in all
	// environments.
	regexes := []string{
		`whisthq/browsers/chrome:current-build`,
		`whisthq/browsers/brave:current-build`,
		`ghcr.io/whisthq/.*/browsers/chrome:current-build`,
		`ghcr.io/whisthq/.*/browsers/brave:current-build`,
		`ghcr.io/whisthq/.*`,
		`.*whisthq.*`,
	}

	image := dockerImageFromRegexes(globalCtx, client, regexes)
	if image == "" {
		return utils.MakeError("Couldn't find a suitable image")
	}

	// It turns out the second mandelbox (including starting the Whist
	// applicaiton) is a bit slow still. Therefore, we do this warmup _twice_.
	warmupCount := 2
	for iter := 1; iter <= warmupCount; iter++ {
		logger.Infof("Staring warmup iteration %v of %v", iter, warmupCount)

		// At this point, we've found an image, so we just need to start the container.

		containerName := utils.PlaceholderWarmupUUID()

		mandelbox := mandelboxData.New(context.Background(), goroutineTracker, mandelboxtypes.MandelboxID(containerName))
		defer mandelbox.Close()

		// Do all startup tasks that can be done before Docker container creation in
		// parallel, stopping at the first error encountered
		preCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

		// Assign port bindings for the mandelbox (necessary for
		// `mandelbox.WriteMandelboxParams()`, though we don't need to actually pass them
		// into the mandelbox)
		preCreateGroup.Go(func() error {
			if err := mandelbox.AssignPortBindings([]portbindings.PortBinding{
				{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
				{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
				{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
			}); err != nil {
				return utils.MakeError("Error assigning port bindings: %s", err)
			}

			return nil
		})

		// Initialize Uinput devices for the mandelbox (necessary for the `update-xorg-conf` service to work)
		var devices []dockercontainer.DeviceMapping
		preCreateGroup.Go(func() error {
			if err := mandelbox.InitializeUinputDevices(goroutineTracker); err != nil {
				return utils.MakeError("Error initializing uinput devices: %s", err)
			}

			devices = mandelbox.GetDeviceMappings()
			return nil
		})

		// Allocate a TTY and GPU
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
			return nil
		})

		err := preCreateGroup.Wait()
		if err != nil {
			return utils.MakeError("Error warming up host service: %v", err)
		}

		aesKey := utils.RandHex(16)
		config := dockercontainer.Config{
			Env: []string{
				utils.Sprintf("WHIST_AES_KEY=%s", aesKey),
				utils.Sprintf("NVIDIA_VISIBLE_DEVICES=%v", "all"),
				"NVIDIA_DRIVER_CAPABILITIES=all",
			},
			Image:        image,
			AttachStdin:  true,
			AttachStdout: true,
			AttachStderr: true,
			Tty:          true,
		}

		tmpfs := make(map[string]string)
		tmpfs["/run"] = "size=52428800"
		tmpfs["/run/lock"] = "size=52428800"

		hostConfig := dockercontainer.HostConfig{
			Binds: []string{
				"/sys/fs/cgroup:/sys/fs/cgroup:ro",
				utils.Sprintf("/whist/%s/mandelboxResourceMappings:/whist/resourceMappings", containerName),
				utils.Sprintf("%s%v/sockets:/tmp/sockets", utils.TempDir, mandelbox.GetID()),
				utils.Sprintf("%slogs/%v/host-service-warmup-%d:/var/log/whist", utils.TempDir, mandelbox.GetID(), iter),
				"/run/udev/data:/run/udev/data:ro",
				utils.Sprintf("/whist/%s/userConfigs/unpacked_configs:/whist/userConfigs:rshared", containerName),
			},
			CapDrop: strslice.StrSlice{"ALL"},
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
				// NOTE THAT CAP_SYS_NICE IS NOT ENABLED BY DEFAULT BY DOCKER --- THIS IS OUR DOING
				"SYS_NICE",
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
		}

		createBody, err := client.ContainerCreate(globalCtx, &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, containerName.String())
		if err != nil {
			return utils.MakeError("Error running `create` for %s:\n%s", containerName, err)
		}
		logger.Infof("warmUpDockerClient(): Created container %s with image %s", containerName, image)

		postCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

		postCreateGroup.Go(func() error {
			if err := mandelbox.WriteMandelboxParams(); err != nil {
				return utils.MakeError("Error writing parameters for mandelbox: %s", err)
			}
			return nil
		})

		postCreateGroup.Go(func() error {
			err = client.ContainerStart(globalCtx, createBody.ID, dockertypes.ContainerStartOptions{})
			if err != nil {
				return utils.MakeError("Error running `start` for %s:\n%s", containerName, err)
			}

			logger.Infof("warmUpDockerClient(): Started container %s with image %s", containerName, image)
			return nil
		})

		err = postCreateGroup.Wait()
		if err != nil {
			return utils.MakeError("Error warming up host service: %v", err)
		}

		// Unblocks whist-startup.sh to start symlink loaded user configs
		err = mandelbox.MarkParamsReady()
		if err != nil {
			return utils.MakeError("Error marking mandelbox params as ready: %s", err)
		}
		err = mandelbox.MarkConfigReady()
		if err != nil {
			return utils.MakeError("Error marking mandelbox configs as ready: %s", err)
		}

		logger.Infof("Waiting for whist application to warm up...")

		if err = utils.WaitForFileCreation(utils.Sprintf("/whist/%s/mandelboxResourceMappings/", containerName), "done_sleeping_until_X_clients", time.Minute*5, nil); err != nil {
			return utils.MakeError("Error warming up whist application: %s", err)
		}

		logger.Infof("Finished waiting for whist application to warm up.")

		time.Sleep(5 * time.Second)

		stopTimeout := 30 * time.Second
		_ = client.ContainerStop(globalCtx, createBody.ID, &stopTimeout)
		err = client.ContainerRemove(globalCtx, createBody.ID, dockertypes.ContainerRemoveOptions{Force: true})
		if err != nil {
			return utils.MakeError("Error running `remove` for %s:\n%s", containerName, err)
		}
		logger.Infof("warmUpDockerClient(): Removed container %s with image %s", containerName, image)

		// Close the mandelbox, and wait 5 seconds so we don't pollute the logs with
		// cleanup after the event loop starts.
		mandelbox.Close()
		time.Sleep(5 * time.Second)

		logger.Infof("Finished warmup iteration %v of %v", iter, warmupCount)
	}

	return nil
}

// Drain and shutdown the host service
func drainAndShutdown(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	logger.Infof("Got a DrainAndShutdownRequest... cancelling the global context.")
	shutdownInstanceOnExit = true
	globalCancel()
}

// ------------------------------------
// Mandelbox event handlers
// ------------------------------------

// SpinUpMandelbox is the request used to create a mandelbox on this host.
func SpinUpMandelbox(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient, sub *subscriptions.MandelboxEvent, transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest, transportMapLock *sync.Mutex) {

	logAndReturnError := func(fmt string, v ...interface{}) {
		err := utils.MakeError("SpinUpMandelbox(): "+fmt, v...)
		logger.Error(err)
		metrics.Increment("ErrorRate")
	}

	// mandelboxSubscription is the pubsub event received from Hasura.
	mandelboxSubscription := sub.MandelboxInfo[0]
	req, AppName := getAppName(mandelboxSubscription.ID, transportRequestMap, transportMapLock)

	logger.Infof("SpinUpMandelbox(): spinup started for mandelbox %s", mandelboxSubscription.ID)

	// Then, verify that we are expecting this user to request a mandelbox.
	err := dbdriver.VerifyAllocatedMandelbox(mandelboxSubscription.UserID, mandelboxSubscription.ID)

	if err != nil {
		logAndReturnError("Unable to spin up mandelbox: %s", err)
		return
	}

	// If so, create the mandelbox object.
	mandelbox := mandelboxData.New(context.Background(), goroutineTracker, mandelboxSubscription.ID)
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

	mandelbox.AssignToUser(mandelboxSubscription.UserID)
	mandelbox.SetAppName(AppName)
	logger.Infof("SpinUpMandelbox(): Successfully assigned mandelbox %s to user %s", mandelboxSubscription.ID, mandelboxSubscription.UserID)

	// Begin loading user configs in parallel with the rest of the mandelbox startup procedure.
	sendEncryptionInfoChan, configDownloadErrChan := mandelbox.StartLoadingUserConfigs(globalCtx, globalCancel, goroutineTracker)
	defer close(sendEncryptionInfoChan)

	// Do all startup tasks that can be done before Docker container creation in
	// parallel, stopping at the first error encountered
	preCreateGroup, _ := errgroup.WithContext(mandelbox.GetContext())

	// Request port bindings for the mandelbox.
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

		var err32262, err32263, err32273 error
		hostPortForTCP32262, err32262 = mandelbox.GetHostPort(32262, portbindings.TransportProtocolTCP)
		hostPortForUDP32263, err32263 = mandelbox.GetHostPort(32263, portbindings.TransportProtocolUDP)
		hostPortForTCP32273, err32273 = mandelbox.GetHostPort(32273, portbindings.TransportProtocolTCP)
		if err32262 != nil || err32263 != nil || err32273 != nil {
			return utils.MakeError("Couldn't return host port bindings.")
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

		logger.Infof("SpinUpMandelbox(): successfully allocated TTY %v and assigned GPU %v for mandelbox %s", mandelbox.GetTTY(), mandelbox.GetGPU(), mandelboxSubscription.ID)
		return nil
	})

	err = preCreateGroup.Wait()
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	// We need to compute the docker image to use for this mandelbox. In local
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

	// We now create the underlying docker container for this mandelbox.
	exposedPorts := make(dockernat.PortSet)
	exposedPorts[dockernat.Port("32262/tcp")] = struct{}{}
	exposedPorts[dockernat.Port("32263/udp")] = struct{}{}
	exposedPorts[dockernat.Port("32273/tcp")] = struct{}{}

	aesKey := utils.RandHex(16)
	envs := []string{
		utils.Sprintf("WHIST_AES_KEY=%s", aesKey),
		utils.Sprintf("NVIDIA_VISIBLE_DEVICES=%v", "all"),
		"NVIDIA_DRIVER_CAPABILITIES=all",
		utils.Sprintf("SENTRY_ENV=%s", metadata.GetAppEnvironment()),
		utils.Sprintf("WHIST_INITIAL_USER_DATA_FILE=%v%v", mandelboxData.UserInitialBrowserDir, mandelboxData.UserInitialBrowserFile),
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

	// Sanitize the received session ID. First, verify that the sessionID is a valid timestamp string.
	// If not, use a new timestamp.
	_, err = strconv.ParseInt(mandelboxSubscription.SessionID, 10, 64)
	if err != nil {
		sessionID := strconv.FormatInt(time.Now().UnixMilli(), 10)
		logger.Warningf("Malformed session ID: %v Using new session ID: %v", mandelboxSubscription.SessionID, sessionID)
		mandelboxSubscription.SessionID = sessionID
	}

	mandelbox.SetSessionID(mandelboxtypes.SessionID(mandelboxSubscription.SessionID))

	hostConfig := dockercontainer.HostConfig{
		Binds: []string{
			"/sys/fs/cgroup:/sys/fs/cgroup:ro",
			utils.Sprintf("/whist/%s/mandelboxResourceMappings:/whist/resourceMappings", mandelbox.GetID()),
			utils.Sprintf("%s%s/sockets:/tmp/sockets", utils.TempDir, mandelbox.GetID()),
			utils.Sprintf("%slogs/%s/%s:/var/log/whist", utils.TempDir, mandelbox.GetID(), mandelboxSubscription.SessionID),
			"/run/udev/data:/run/udev/data:ro",
			utils.Sprintf("/whist/%s/userConfigs/unpacked_configs:/whist/userConfigs:rshared", mandelbox.GetID()),
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
			// NOTE THAT CAP_SYS_NICE IS NOT ENABLED BY DEFAULT BY DOCKER --- THIS IS OUR DOING
			"SYS_NICE",
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
	mandelboxName := utils.Sprintf("%s-%s", AppName, mandelboxSubscription.ID)
	re := regexp.MustCompile(`[^a-zA-Z0-9_.-]`)
	mandelboxName = re.ReplaceAllString(mandelboxName, "-")

	dockerBody, err := dockerClient.ContainerCreate(mandelbox.GetContext(), &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, mandelboxName)
	if err != nil {
		logAndReturnError("Error running `create` for %s:\n%s", mandelbox.GetID(), err)
		return
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

		// Let server protocol wait 30 seconds by default before client connects.
		// However, in a local environment, we set the default to an effectively
		// infinite value.
		protocolTimeout := 30
		if metadata.IsLocalEnv() {
			protocolTimeout = -1
		}

		if err := mandelbox.WriteProtocolTimeout(protocolTimeout); err != nil {
			return utils.MakeError("Error writing protocol timeout: %s", err)
		}

		logger.Infof("SpinUpMandelbox(): Successfully wrote parameters for mandelbox %s", mandelboxSubscription.ID)
		return nil
	})

	// Start docker container
	postCreateGroup.Go(func() error {
		err = dockerClient.ContainerStart(mandelbox.GetContext(), string(dockerID), dockertypes.ContainerStartOptions{})
		if err != nil {
			return utils.MakeError("Error starting mandelbox with runtime ID %s and MandelboxID %s: %s", dockerID, mandelboxSubscription.ID, err)
		}

		logger.Infof("SpinUpMandelbox(): Successfully started mandelbox %s with ID %s", mandelboxName, mandelboxSubscription.ID)
		return nil
	})

	// Mark as ready only when all startup tasks have completed
	err = postCreateGroup.Wait()
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	err = mandelbox.MarkParamsReady()
	if err != nil {
		logAndReturnError("Error marking mandelbox %s as ready to start A/V + display: %s", mandelboxSubscription.ID, err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully marked mandelbox %s params as ready. A/V and display services can soon start.", mandelboxSubscription.ID)

	logger.Infof("SpinUpMandelbox(): Waiting for config encryption token from client...")

	if req == nil {
		// Receive the json transport request from the client via the httpserver.
		jsonChan := getJSONTransportRequestChannel(mandelboxSubscription.ID, transportRequestMap, transportMapLock)

		// Set a timeout for the json transport request to prevent the mandelbox from waiting forever.
		select {
		case transportRequest := <-jsonChan:
			req = transportRequest
		case <-time.After(1 * time.Minute):
			// Clean up the mandelbox if the time out limit is reached.
			// TODO: why do we call the die handler here instead of just returning (and letting the deferred mandelbox.close() take care of things)?
			mandelboxDieHandler(string(dockerID), transportRequestMap, transportMapLock, dockerClient)
			logAndReturnError("Timed out waiting for config encryption token for user %s for mandelbox %s", mandelbox.GetUserID(), mandelbox.GetID())
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

	// Write the user's initial browser data
	logger.Infof("SpinUpMandelbox(): Beginning storing user initial browser data for mandelbox %s", mandelboxSubscription.ID)
	err = mandelbox.WriteUserInitialBrowserData(mandelboxData.BrowserData{
		CookiesJSON:   req.CookiesJSON,
		BookmarksJSON: req.BookmarksJSON,
		Extensions:    req.Extensions,
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

	// Don't wait for whist application to start up in local environment. We do
	// this because in local environments, we want to provide the developer a
	// shell into the container immediately, not conditional on everything
	// starting up properly.
	if !metadata.IsLocalEnv() {
		logger.Infof("SpinUpMandelbox(): Waiting for mandelbox %s whist application to start up...", mandelboxSubscription.ID)
		if err = utils.WaitForFileCreation(utils.Sprintf("/whist/%s/mandelboxResourceMappings/", mandelboxSubscription.ID), "done_sleeping_until_X_clients", time.Second*20, nil); err != nil {
			logAndReturnError("Error waiting for mandelbox %s whist application to start: %s", mandelboxSubscription.ID, err)
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
		AesKey:              aesKey,
	}
	req.ReturnResult(result, nil)
	logger.Infof("SpinUpMandelbox(): Finished starting up mandelbox %s", mandelbox.GetID())
}

// Handle tasks to be completed when a mandelbox dies
func mandelboxDieHandler(id string, transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest, transportMapLock *sync.Mutex, dockerClient dockerclient.CommonAPIClient) {
	// Exit if we are not dealing with a Whist mandelbox, or if it has already
	// been closed (via a call to Close() or a context cancellation).
	mandelbox, err := mandelboxData.LookUpByDockerID(mandelboxtypes.DockerID(id))
	if err != nil {
		logger.Infof("mandelboxDieHandler(): %s", err)
		return
	}

	// Clean up this user from the JSON transport request map.
	mandelboxID := mandelbox.GetID()
	transportMapLock.Lock()
	transportRequestMap[mandelboxID] = nil
	transportMapLock.Unlock()

	// Gracefully shut down the mandelbox docker container
	stopTimeout := 30 * time.Second
	err = dockerClient.ContainerStop(mandelbox.GetContext(), id, &stopTimeout)

	if err != nil {
		logger.Errorf("Failed to gracefully stop mandelbox docker container.")
		metrics.Increment("ErrorRate")
	}

	mandelbox.Close()
}

// ---------------------------
// Service shutdown and initialization
// ---------------------------

//go:embed mandelbox-apparmor-profile
var appArmorProfile string

// Create and register the AppArmor profile for Docker to use with
// mandelboxes. These do not persist, so must be done at service
// startup.
func initializeAppArmor(globalCancel context.CancelFunc) {
	cmd := exec.Command("apparmor_parser", "-Kr")
	stdin, err := cmd.StdinPipe()
	if err != nil {
		logger.Panicf(globalCancel, "Unable to attach to process 'stdin' pipe. Error: %v", err)
	}

	go func() {
		defer stdin.Close()
		io.WriteString(stdin, appArmorProfile)
	}()

	err = cmd.Run()
	if err != nil {
		logger.Panicf(globalCancel, "Unable to register AppArmor profile. Error: %v", err)
	}
}

// Create the directory used to store the mandelbox resource allocations
// (e.g. TTYs) on disk
func initializeFilesystem(globalCancel context.CancelFunc) {
	// check if "/whist" already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(utils.WhistDir); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf(globalCancel, "Directory %s already exists!", utils.WhistDir)
		} else {
			logger.Panicf(globalCancel, "Could not make directory %s because of error %v", utils.WhistDir, err)
		}
	}

	// Create the whist directory and make it non-root user owned so that
	// non-root users in mandelboxes can access files within (especially user
	// configs).
	err := os.MkdirAll(utils.WhistDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", utils.WhistDir, err)
	}
	cmd := exec.Command("chown", "-R", "ubuntu", utils.WhistDir)
	cmd.Run()

	// Create whist-private directory
	err = os.MkdirAll(utils.WhistPrivateDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", utils.WhistPrivateDir, err)
	}

	// Create whist temp directory (only let root read and write this, since it
	// contains logs and uinput sockets).
	err = os.MkdirAll(utils.TempDir, 0600)
	if err != nil {
		logger.Panicf(globalCancel, "Could not mkdir path %s. Error: %s", utils.TempDir, err)
	}
}

// Delete the directory used to store the mandelbox resource allocations (e.g.
// TTYs) on disk, as well as the directory used to store the SSL certificate we
// use for the httpserver, and our temporary directory.
func uninitializeFilesystem() {
	logger.Infof("removing all files")
	err := os.RemoveAll(utils.WhistDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.WhistDir, err)
		metrics.Increment("ErrorRate")
	} else {
		logger.Infof("Successfully deleted directory %s\n", utils.WhistDir)
	}

	err = os.RemoveAll(utils.WhistPrivateDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.WhistPrivateDir, err)
		metrics.Increment("ErrorRate")
	} else {
		logger.Infof("Successfully deleted directory %s\n", utils.WhistPrivateDir)
	}

	err = os.RemoveAll(utils.TempDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.TempDir, err)
		metrics.Increment("ErrorRate")
	} else {
		logger.Infof("Successfully deleted directory %s\n", utils.TempDir)
	}
}

func main() {
	// We create a global context (i.e. for the entire host service) that can be
	// cancelled if the entire program needs to terminate. We also create a
	// WaitGroup for all goroutines to tell us when they've stopped (if the
	// context gets cancelled). Finally, we defer a function which cancels the
	// global context if necessary, logs any panic we might be recovering from,
	// and cleans up after the entire host service (although some aspects of
	// cleanup may not seem necessary on a production instance, they are
	// immensely helpful to prevent the host service from clogging up the local
	// filesystem during development). After the permissions check, the creation
	// of this context and WaitGroup, and the following defer must be the first
	// statements in main().
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}
	defer func() {
		// This function cleanly shuts down the Whist Host Service. Note that
		// besides the host machine itself being forcefully shut down, this
		// deferred function from main() should be the _only_ way that the host
		// service exits. In particular, it should be as a result of a panic() in
		// main, the global context being cancelled, or a Ctrl+C interrupt.

		// Note that this function, while nontrivial, has intentionally been left
		// as part of main() for the following reasons:
		//   1. To prevent it being called from anywhere else accidentally
		//   2. To keep the entire program control flow clearly in main().

		// Catch any panics that might have originated in main() or one of its
		// direct children.
		r := recover()
		if r != nil {
			logger.Infof("Shutting down host service after caught panic in main(): %v", r)
		} else {
			logger.Infof("Beginning host service shutdown procedure...")
		}

		// Cancel the global context, if it hasn't already been cancelled.
		globalCancel()

		// Wait for all goroutines to stop, so we can run the rest of the cleanup
		// process.
		utils.WaitWithDebugPrints(&goroutineTracker, 2*time.Minute, 2)

		// Stop processing new events
		close(eventLoopKeepalive)

		uninitializeFilesystem()

		// Remove our row from the database and close out the database driver.
		dbdriver.Close()

		// Close out our metrics collection.
		metrics.Close()

		// Drain to our remote logging providers, but don't yet stop recording new
		// events, in case the shutdown fails.
		logger.FlushLogzio()
		logger.FlushSentry()

		logger.Info("Finished host service shutdown procedure. Finally exiting...")
		if shutdownInstanceOnExit {
			if err := exec.Command("shutdown", "now").Run(); err != nil {
				if strings.TrimSpace(err.Error()) == "signal: terminated" {
					// The instance seems to shut down even when this error is fired, so
					// we'll just ignore it. We still `logger.Info()` it just in case.
					logger.Infof("Shutdown command returned 'signal: terminated' error. Ignoring it.")
				} else {
					logger.Errorf("Couldn't shut down instance: %s", err)
					metrics.Increment("ErrorRate")
				}
			}
		}

		// Shut down the logging infrastructure (including re-draining the queues).
		logger.Close()

		os.Exit(0)
	}()

	// Log the Git commit of the running executable
	logger.Info("Host Service Version: %s", metadata.GetGitCommit())

	// Initialize the database driver, if necessary (the `dbdriver` package
	// takes care of the "if necessary" part).
	if err := dbdriver.Initialize(globalCtx, globalCancel, &goroutineTracker); err != nil {
		logger.Panic(globalCancel, err)
	}

	// Log the instance name we're running on
	instanceName, err := aws.GetInstanceName()
	if err != nil {
		logger.Panic(globalCancel, err)
	}
	logger.Infof("Running on instance name: %s", instanceName)

	initializeAppArmor(globalCancel)

	initializeFilesystem(globalCancel)

	// Start Docker
	dockerClient, err := createDockerClient()
	if err != nil {
		logger.Panic(globalCancel, err)
	}

	if !metadata.IsLocalEnv() {
		if err := warmUpDockerClient(globalCtx, globalCancel, &goroutineTracker, dockerClient); err != nil {
			logger.Panicf(globalCancel, "Error warming up docker client: %s", err)
		}
	}

	if err := dbdriver.RegisterInstance(); err != nil {
		// If the instance starts up and sees its status as unresponsive or
		// draining, the webserver doesn't want it anymore so we should shut down.

		// TODO: make this a bit more robust
		if !metadata.IsLocalEnv() && (strings.Contains(err.Error(), string(dbdriver.InstanceStatusUnresponsive)) ||
			strings.Contains(err.Error(), string(dbdriver.InstanceStatusDraining))) {
			logger.Infof("Instance wasn't registered in database because we found ourselves already marked draining or unresponsive. Shutting down.... Error: %s", err)
			shutdownInstanceOnExit = true
			globalCancel()
		} else {
			logger.Panic(globalCancel, err)
		}
	}

	// Now we start all the goroutines that actually do work.

	// Start the HTTP server and listen for events
	httpServerEvents, err := StartHTTPServer(globalCtx, globalCancel, &goroutineTracker)
	if err != nil {
		logger.Panic(globalCancel, err)
	}

	// Start database subscription client
	instanceName, err = aws.GetInstanceName()
	if err != nil {
		logger.Errorf("Can't get AWS Instance Name to start database subscriptions. Error: %s", err)
		metrics.Increment("ErrorRate")
	}
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)

	subscriptionClient := &subscriptions.SubscriptionClient{}
	subscriptions.SetupHostSubscriptions(string(instanceName), subscriptionClient)
	subscriptions.Start(subscriptionClient, globalCtx, &goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscriptions. Error: %s", err)
	}

	// Start main event loop. Note that we don't track this goroutine, but
	// instead control its lifetime with `eventLoopKeepAlive`. This is because it
	// needs to stay alive after the global context is cancelled, so we can
	// process mandelbox death events.
	go eventLoopGoroutine(globalCtx, globalCancel, &goroutineTracker, dockerClient, httpServerEvents, subscriptionEvents)

	// Register a signal handler for Ctrl-C so that we cleanup if Ctrl-C is pressed.
	sigChan := make(chan os.Signal, 2)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)

	// Wait for either the global context to get cancelled by a worker goroutine,
	// or for us to receive an interrupt. This needs to be the end of main().
	select {
	case <-sigChan:
		logger.Infof("Got an interrupt or SIGTERM")
	case <-globalCtx.Done():
		logger.Infof("Global context cancelled!")
	}
}

// As long as this channel is blocking, we continue processing events
// (including Docker events).
var eventLoopKeepalive = make(chan interface{}, 1)

func eventLoopGoroutine(globalCtx context.Context, globalCancel context.CancelFunc,
	goroutineTracker *sync.WaitGroup, dockerClient dockerclient.CommonAPIClient,
	httpServerEvents <-chan ServerRequest, subscriptionEvents <-chan subscriptions.SubscriptionEvent) {
	// Note that we don't use globalCtx for the docker Context, since we still
	// wish to process Docker events after the global context is cancelled.
	dockerContext, dockerContextCancel := context.WithCancel(context.Background())
	defer dockerContextCancel()

	// Create filter for which docker events we care about
	filters := dockerfilters.NewArgs()
	filters.Add("type", dockerevents.ContainerEventType)
	eventOptions := dockertypes.EventsOptions{
		Filters: filters,
	}

	// We use this lock to protect the transportRequestMap
	transportMapLock := &sync.Mutex{}

	// Note: If a mandelbox suffers a bug, or fails to start correctly
	// these channels will become a memory leak.
	transportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)

	// In the following loop, this var determines whether to re-initialize the
	// Docker event stream. This is necessary because the Docker event stream
	// needs to be reopened after any error is sent over the error channel.
	needToReinitDockerEventStream := false
	dockerevents, dockererrs := dockerClient.Events(dockerContext, eventOptions)
	logger.Info("Initialized docker event stream.")
	logger.Info("Entering event loop...")

	// The actual event loop
	for {
		if needToReinitDockerEventStream {
			dockerevents, dockererrs = dockerClient.Events(dockerContext, eventOptions)
			needToReinitDockerEventStream = false
			logger.Info("Re-initialized docker event stream.")
		}

		select {
		case <-eventLoopKeepalive:
			logger.Infof("Leaving main event loop...")
			return

		case err := <-dockererrs:
			needToReinitDockerEventStream = true
			switch {
			case err == nil:
				logger.Info("We got a nil error over the Docker event stream. This might indicate an error inside the docker go library. Ignoring it and proceeding normally...")
				continue
			case err == io.EOF:
				logger.Panicf(globalCancel, "Docker event stream has been completely read.")
			case dockerclient.IsErrConnectionFailed(err):
				// This means "Cannot connect to the Docker daemon..."
				logger.Panicf(globalCancel, "Got error \"%v\". Could not connect to the Docker daemon.", err)
			default:
				if !strings.HasSuffix(strings.ToLower(err.Error()), "context canceled") {
					logger.Panicf(globalCancel, "Got an unknown error from the Docker event stream: %v", err)
				}
				return
			}

		case dockerevent := <-dockerevents:
			logger.Info("dockerevent: %s for %s %s\n", dockerevent.Action, dockerevent.Type, dockerevent.ID)
			if dockerevent.Action == "die" {
				mandelboxDieHandler(dockerevent.ID, transportRequestMap, transportMapLock, dockerClient)
			}

		// It may seem silly to just launch goroutines to handle these
		// serverevents, but we aim to keep the high-level flow control and handling
		// in this package, and the low-level authentication, parsing, etc. of
		// requests in `httpserver`.
		case serverevent := <-httpServerEvents:
			switch serverevent := serverevent.(type) {
			// TODO: actually handle panics in these goroutines
			case *JSONTransportRequest:
				if !metadata.IsLocalEnvWithoutDB() {
					// Handle JSON transport validation on a separate goroutine
					go handleJSONTransportRequest(serverevent, transportRequestMap, transportMapLock)
				} else {
					// If running on a local environment, disable any pubsub logic. We have to create a subscription request
					// that mocks the Hasura subscription event. Doing this avoids the need of setting up a Hasura server and
					// postgres database on the development instance.
					jsonReq := serverevent

					userID, err := metadata.GetUserID()
					if err != nil {
						logger.Errorf("Error getting userID, %v", err)
						metrics.Increment("ErrorRate")
					}

					instanceName, err := aws.GetInstanceName()
					if err != nil {
						logger.Errorf("Error getting instance name from AWS, %v", err)
						metrics.Increment("ErrorRate")
					}
					// Create a mandelbox object as would be received from a Hasura subscription.
					mandelbox := subscriptions.Mandelbox{
						InstanceName: string(instanceName),
						ID:           jsonReq.MandelboxID,
						SessionID:    "1234",
						UserID:       userID,
					}
					subscriptionEvent := subscriptions.MandelboxEvent{
						MandelboxInfo: []subscriptions.Mandelbox{mandelbox},
					}

					// Launch both the JSON transport handler and the SpinUpMandelbox goroutines.
					go handleJSONTransportRequest(serverevent, transportRequestMap, transportMapLock)
					go SpinUpMandelbox(globalCtx, globalCancel, goroutineTracker, dockerClient, &subscriptionEvent, transportRequestMap, transportMapLock)
				}
			default:
				if serverevent != nil {
					err := utils.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
					logger.Error(err)
					serverevent.ReturnResult("", err)
				}
			}

		case subscriptionEvent := <-subscriptionEvents:
			switch subscriptionEvent := subscriptionEvent.(type) {
			// TODO: actually handle panics in these goroutines
			case *subscriptions.MandelboxEvent:
				go SpinUpMandelbox(globalCtx, globalCancel, goroutineTracker, dockerClient,
					subscriptionEvent, transportRequestMap, transportMapLock)

			case *subscriptions.InstanceEvent:
				// Don't do this in a separate goroutine, since there's no reason to.
				drainAndShutdown(globalCtx, globalCancel, goroutineTracker)

			default:
				if subscriptionEvent != nil {
					err := utils.MakeError("Unimplemented handling of subscription event [type: %T]: %v", subscriptionEvent, subscriptionEvent)
					logger.Error(err)
				}
			}
		}
	}
}
