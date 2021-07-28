package main

import (
	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via Sentry. Instead, use the
	// fractallogger package imported below as `logger`.

	"context"
	"io"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"regexp"
	"strings"
	"sync"
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry. For the same reason, we make sure not
	// to import the fmt package either, instead separating required
	// functionality in this imported package as well.
	logger "github.com/fractal/fractal/host-service/fractallogger"

	"github.com/fractal/fractal/host-service/auth"
	"github.com/fractal/fractal/host-service/dbdriver"
	"github.com/fractal/fractal/host-service/mandelbox"
	"github.com/fractal/fractal/host-service/mandelbox/portbindings"
	"github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/metadata/aws"
	"github.com/fractal/fractal/host-service/metrics"
	"github.com/fractal/fractal/host-service/utils"

	dockertypes "github.com/docker/docker/api/types"
	dockercontainer "github.com/docker/docker/api/types/container"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/api/types/strslice"
	dockerclient "github.com/docker/docker/client"
	dockernat "github.com/docker/go-connections/nat"
	dockerunits "github.com/docker/go-units"
	v1 "github.com/opencontainers/image-spec/specs-go/v1"

	"github.com/golang-jwt/jwt"
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

// Start the Docker daemon ourselves, to have control over all Docker
// containers running on the host.
func startDockerDaemon(globalCancel context.CancelFunc) {
	cmd := exec.Command("/usr/bin/systemctl", "start", "docker")
	err := cmd.Run()
	if err != nil {
		logger.Panicf(globalCancel, "Unable to start Docker daemon. Error: %v", err)
	} else {
		logger.Info("Successfully started the Docker daemon ourselves.")
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
func warmUpDockerClient(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, client *dockerclient.Client) error {
	imageFilters := dockerfilters.NewArgs(
		dockerfilters.KeyValuePair{Key: "dangling", Value: "false"},
	)
	images, err := client.ImageList(globalCtx, dockertypes.ImageListOptions{All: false, Filters: imageFilters})
	if err != nil {
		return err
	}

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
		`fractal/browsers/chrome:current-build`,
		`ghcr.io/fractal/*/browsers/chrome:current-build`,
		`ghcr.io/fractal/*`,
		`*fractal*`,
	}

	findImageByRegex := func(nameRegex string) string {
		regex := regexp.MustCompile(nameRegex)
		for _, img := range images {
			for _, tag := range img.RepoTags {
				if regex.MatchString(tag) {
					return tag
				}
			}
		}
		return ""
	}

	image := func() string {
		for _, r := range regexes {
			if match := findImageByRegex(r); match != "" {
				return match
			}
		}
		return ""
	}()
	if image == "" {
		return utils.MakeError("Couldn't find a suitable image")
	}

	// It turns out the second mandelbox (including starting the Fractal
	// applicaiton) is a bit slow still. Therefore, we do this warmup _twice_.
	warmupCount := 2
	for iter := 1; iter <= warmupCount; iter++ {
		logger.Infof("Staring warmup iteration %v of %v", iter, warmupCount)

		// At this point, we've found an image, so we just need to start the container.

		containerName := "host-service-warmup"
		fc := mandelbox.New(context.Background(), goroutineTracker, types.MandelboxID(containerName))
		defer fc.Close()

		// Assign port bindings for the mandelbox (necessary for
		// `fc.WriteMandelboxParams()`, though we don't need to actually pass them
		// into the mandelbox)
		if err := fc.AssignPortBindings([]portbindings.PortBinding{
			{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
			{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
			{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
		}); err != nil {
			return utils.MakeError("Error assigning port bindings: %s", err)
		}

		// Initialize Uinput devices for the mandelbox (necessary for the `update-xorg-conf` service to work)
		if err := fc.InitializeUinputDevices(goroutineTracker); err != nil {
			return utils.MakeError("Error initializing uinput devices: %s", err)
		}
		devices := fc.GetDeviceMappings()

		// Allocate a TTY and GPU
		if err := fc.InitializeTTY(); err != nil {
			return utils.MakeError("Error initializing TTY: %s", err)
		}
		if err := fc.AssignGPU(); err != nil {
			return utils.MakeError("Error assigning GPU: %s", err)
		}

		aesKey := utils.RandHex(16)
		config := dockercontainer.Config{
			Env: []string{
				utils.Sprintf("FRACTAL_AES_KEY=%s", aesKey),
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
				utils.Sprintf("/fractal/%s/mandelboxResourceMappings:/fractal/resourceMappings", containerName),
				utils.Sprintf("/fractal/temp/%s/sockets:/tmp/sockets", fc.GetMandelboxID()),
				"/run/udev/data:/run/udev/data:ro",
				utils.Sprintf("/fractal/%s/userConfigs/unpacked_configs:/fractal/userConfigs:rshared", containerName),
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

		createBody, err := client.ContainerCreate(globalCtx, &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, containerName)
		if err != nil {
			return utils.MakeError("Error running `create` for %s:\n%s", containerName, err)
		}
		logger.Infof("warmUpDockerClient(): Created container %s with image %s", containerName, image)

		if err := fc.WriteMandelboxParams(); err != nil {
			return utils.MakeError("Error writing parameters for mandelbox: %s", err)
		}
		err = fc.MarkReady()
		if err != nil {
			return utils.MakeError("Error marking mandelbox as ready: %s", err)
		}

		err = client.ContainerStart(globalCtx, createBody.ID, dockertypes.ContainerStartOptions{})
		if err != nil {
			return utils.MakeError("Error running `start` for %s:\n%s", containerName, err)
		}
		logger.Infof("warmUpDockerClient(): Started container %s with image %s", containerName, image)

		logger.Infof("Waiting for fractal application to warm up...")
		if err = utils.WaitForFileCreation(utils.Sprintf("/fractal/%s/mandelboxResourceMappings/", containerName), "done_sleeping_until_X_clients", time.Minute*5); err != nil {
			return utils.MakeError("Error warming up fractal application: %s", err)
		}
		logger.Infof("Finished waiting for fractal application to warm up.")

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
		fc.Close()
		time.Sleep(5 * time.Second)

		logger.Infof("Finished warmup iteration %v of %v", iter, warmupCount)
	}

	return nil
}

// Drain and shutdown the host service
func drainAndShutdown(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, req *DrainAndShutdownRequest) {
	logger.Infof("Got a DrainAndShutdownRequest... cancelling the global context.")

	// Note that the caller won't actually know if the `shutdown` command failed.
	// This response is just saying that we got the request successfully.
	defer req.ReturnResult("request successful", nil)

	shutdownInstanceOnExit = true
	globalCancel()
}

// ------------------------------------
// Mandelbox event handlers
// ------------------------------------

// SpinUpMandelbox is the request used to create a mandelbox on this host.
func SpinUpMandelbox(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient *dockerclient.Client, req *SpinUpMandelboxRequest) {
	logAndReturnError := func(fmt string, v ...interface{}) {
		err := utils.MakeError("SpinUpMandelbox(): "+fmt, v...)
		logger.Error(err)
		req.ReturnResult("", err)
	}

	// Set up auth
	claims := new(auth.FractalClaims)
	parser := &jwt.Parser{SkipClaimsValidation: true}
	var userID types.UserID

	// Only verify auth in non-local environments
	if !metadata.IsLocalEnv() {
		// Decode the access token without validating any of its claims or
		// verifying its signature because we've already done that. All we want to
		// know is the value of the sub (subject) claim.
		if _, _, err := parser.ParseUnverified(string(req.JwtAccessToken), claims); err != nil {
			logAndReturnError("There was a problem while parsing the access token for the second time: %s", err)
			return
		}
		userID = types.UserID(claims.Subject)
	} else {
		instanceName, err := aws.GetInstanceName()
		if err != nil {
			logAndReturnError("Can't get AWS Instance name for localdev user config userID.")
			return
		}
		userID = types.UserID(utils.Sprintf("localdev_host_service_user_%s", instanceName))
	}

	// Then, verify that we are expecting this user to request a mandelbox.
	err := dbdriver.VerifyAllocatedMandelbox(userID, req.MandelboxID)

	if err != nil {
		logAndReturnError("Unable to spin up mandelbox: %s", err)
		return
	}

	// If so, create the mandelbox object.
	fc := mandelbox.New(context.Background(), goroutineTracker, req.MandelboxID)
	logger.Infof("SpinUpMandelbox(): created Mandelbox object %s", fc.GetMandelboxID())

	// If the creation of the mandelbox fails, we want to clean up after it. We
	// do this by setting `createFailed` to true until all steps are done, and
	// closing the mandelbox's context on function exit if `createFailed` is
	// still set to true.
	var createFailed bool = true
	defer func() {
		if createFailed {
			fc.Close()
		}
	}()

	// Verify that this user sent in a (nontrivial) config encryption token
	if len(req.ConfigEncryptionToken) < 10 {
		logAndReturnError("Unable to spin up mandelbox: trivial config encryption token received.", err)
		return
	}
	fc.SetConfigEncryptionToken(req.ConfigEncryptionToken)

	// Request port bindings for the mandelbox.
	if err := fc.AssignPortBindings([]portbindings.PortBinding{
		{MandelboxPort: 32262, HostPort: 0, BindIP: "", Protocol: "tcp"},
		{MandelboxPort: 32263, HostPort: 0, BindIP: "", Protocol: "udp"},
		{MandelboxPort: 32273, HostPort: 0, BindIP: "", Protocol: "tcp"},
	}); err != nil {
		logAndReturnError("Error assigning port bindings: %s", err)
		return
	}
	logger.Infof("SpinUpMandelbox(): successfully assigned port bindings %v", fc.GetPortBindings())

	hostPortForTCP32262, err32262 := fc.GetHostPort(32262, portbindings.TransportProtocolTCP)
	hostPortForUDP32263, err32263 := fc.GetHostPort(32263, portbindings.TransportProtocolUDP)
	hostPortForTCP32273, err32273 := fc.GetHostPort(32273, portbindings.TransportProtocolTCP)
	if err32262 != nil || err32263 != nil || err32273 != nil {
		logAndReturnError("Couldn't return host port bindings.")
		return
	}

	// Initialize Uinput devices for the mandelbox
	if err := fc.InitializeUinputDevices(goroutineTracker); err != nil {
		logAndReturnError("Error initializing uinput devices: %s", err)
		return
	}
	logger.Infof("SpinUpMandelbox(): successfully initialized uinput devices.")
	devices := fc.GetDeviceMappings()

	// Allocate a TTY for the mandelbox
	if err := fc.InitializeTTY(); err != nil {
		logAndReturnError("Error initializing TTY: %s", err)
		return
	}
	// Assign a GPU to the mandelbox
	if err := fc.AssignGPU(); err != nil {
		logAndReturnError("Error assigning GPU: %s", err)
		return
	}
	logger.Infof("SpinUpMandelbox(): successfully allocated TTY %v and assigned GPU %v for mandelbox %s", fc.GetTTY(), fc.GetGPU(), req.MandelboxID)

	// We need to compute the docker image to use for this mandelbox. In local
	// development, we want to accept any string so that we can run arbitrary
	// containers as mandelboxes, including custom-tagged ones. However, in
	// deployments we only want to accept the specific apps that are enabled, and
	// don't want to leak the fact that we're using containers as the technology
	// for mandelboxes, so we construct the image string ourselves.
	var image string
	if metadata.IsLocalEnv() {
		image = string(req.AppName)
	} else {
		image = utils.Sprintf("ghcr.io/fractal/%s/%s:current-build", metadata.GetAppEnvironmentLowercase(), req.AppName)
	}

	// We now create the underlying docker container for this mandelbox.
	exposedPorts := make(dockernat.PortSet)
	exposedPorts[dockernat.Port("32262/tcp")] = struct{}{}
	exposedPorts[dockernat.Port("32263/udp")] = struct{}{}
	exposedPorts[dockernat.Port("32273/tcp")] = struct{}{}

	aesKey := utils.RandHex(16)
	envs := []string{
		utils.Sprintf("FRACTAL_AES_KEY=%s", aesKey),
		utils.Sprintf("NVIDIA_VISIBLE_DEVICES=%v", "all"),
		"NVIDIA_DRIVER_CAPABILITIES=all",
		utils.Sprintf("SENTRY_ENV=%s", metadata.GetAppEnvironment()),
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

	hostConfig := dockercontainer.HostConfig{
		Binds: []string{
			"/sys/fs/cgroup:/sys/fs/cgroup:ro",
			utils.Sprintf("/fractal/%s/mandelboxResourceMappings:/fractal/resourceMappings", fc.GetMandelboxID()),
			utils.Sprintf("/fractal/temp/%s/sockets:/tmp/sockets", fc.GetMandelboxID()),
			utils.Sprintf("/fractal/temp/logs/%s:/var/log/fractal", fc.GetMandelboxID()),
			"/run/udev/data:/run/udev/data:ro",
			utils.Sprintf("/fractal/%s/userConfigs/unpacked_configs:/fractal/userConfigs:rshared", fc.GetMandelboxID()),
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
	}
	// TODO: investigate whether putting all GPUs in all mandelboxes (i.e. the default here) is beneficial.
	mandelboxName := utils.Sprintf("%s-%s", req.AppName, req.MandelboxID)
	re := regexp.MustCompile(`[^a-zA-Z0-9_.-]`)
	mandelboxName = re.ReplaceAllString(mandelboxName, "-")

	dockerBody, err := dockerClient.ContainerCreate(fc.GetContext(), &config, &hostConfig, nil, &v1.Platform{Architecture: "amd64", OS: "linux"}, mandelboxName)
	if err != nil {
		logAndReturnError("Error running `create` for %s:\n%s", fc.GetMandelboxID(), err)
		return
	}

	logger.Infof("Value returned from ContainerCreate: %#v", dockerBody)
	dockerID := types.DockerID(dockerBody.ID)

	logger.Infof("SpinUpMandelbox(): Successfully ran `create` command and got back runtime ID %s", dockerID)

	err = fc.RegisterCreation(dockerID, req.AppName)
	if err != nil {
		logAndReturnError("Error registering mandelbox creation with runtime ID %s and AppName %s: %s", dockerID, req.AppName, err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully registered mandelbox creation with runtime ID %s and AppName %s", dockerID, req.AppName)

	if err := fc.WriteMandelboxParams(); err != nil {
		logAndReturnError("Error writing mandelbox params: %s", err)
		return
	}
	// Let server protocol wait 30 seconds by default before client connects.
	// However, in a local environment, we set the default to an effectively
	// infinite value.
	protocolTimeout := 30
	if metadata.IsLocalEnv() {
		protocolTimeout = -1
	}
	if err := fc.WriteProtocolTimeout(protocolTimeout); err != nil {
		logAndReturnError("Error writing protocol timeout: %s", err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully wrote parameters for mandelbox %s", req.MandelboxID)

	err = dockerClient.ContainerStart(fc.GetContext(), string(dockerID), dockertypes.ContainerStartOptions{})
	if err != nil {
		logAndReturnError("Error starting mandelbox with runtime ID %s and MandelboxID %s: %s", dockerID, req.MandelboxID, err)
		return
	}
	logger.Infof("SpinUpMandelbox(): Successfully started mandelbox %s with ID %s", mandelboxName, req.MandelboxID)

	result := SpinUpMandelboxRequestResult{
		HostPortForTCP32262: hostPortForTCP32262,
		HostPortForUDP32263: hostPortForUDP32263,
		HostPortForTCP32273: hostPortForTCP32273,
		AesKey:              aesKey,
	}

	fc.AssignToUser(userID)

	err = fc.PopulateUserConfigs()
	if err != nil {
		// Not a fatal error --- we still want to spin up a mandelbox, and we will
		// just use the provided encryption token to save configs when the
		// mandelbox dies.
		logger.Warning(err)
	}

	err = fc.MarkReady()
	if err != nil {
		logAndReturnError("Error marking mandelbox %s as ready: %s", req.MandelboxID, err)
		return
	}

	// Don't wait for fractal application to start up in local environment
	if !metadata.IsLocalEnv() {
		logger.Infof("Waiting for fractal application to start up...")
		if err = utils.WaitForFileCreation(utils.Sprintf("/fractal/%s/mandelboxResourceMappings/", req.MandelboxID), "done_sleeping_until_X_clients", time.Second*20); err != nil {
			logAndReturnError("Error warming up fractal application: %s", err)
			return
		}
		logger.Infof("Finished waiting for fractal application to start up.")
	}

	err = dbdriver.WriteMandelboxStatus(req.MandelboxID, dbdriver.MandelboxStatusRunning)
	if err != nil {
		logAndReturnError("Error marking mandelbox running: %s", err)
		return
	}

	// Mark mandelbox creation as successful, preventing cleanup on function
	// termination.
	createFailed = false

	req.ReturnResult(result, nil)
	logger.Infof("SpinUpMandelbox(): Finished starting up mandelbox %s", fc.GetMandelboxID())
}

// Handle tasks to be completed when a mandelbox dies
func mandelboxDieHandler(id string) {
	// Exit if we are not dealing with a Fractal mandelbox, or if it has already
	// been closed (via a call to Close() or a context cancellation).
	fc, err := mandelbox.LookUpByDockerID(types.DockerID(id))
	if err != nil {
		logger.Infof("mandelboxDieHandler(): %s", err)
		return
	}

	fc.Close()
}

// ---------------------------
// Service shutdown and initialization
// ---------------------------

// Create the directory used to store the mandelbox resource allocations
// (e.g. TTYs) on disk
func initializeFilesystem(globalCancel context.CancelFunc) {
	// check if "/fractal" already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(utils.FractalDir); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf(globalCancel, "Directory %s already exists!", utils.FractalDir)
		} else {
			logger.Panicf(globalCancel, "Could not make directory %s because of error %v", utils.FractalDir, err)
		}
	}

	// Create the fractal directory and make it non-root user owned so that
	// non-root users in mandelboxes can access files within (especially user
	// configs). We do this in a deferred function so that any subdirectories
	// created later in this function are also covered.
	err := os.MkdirAll(utils.FractalDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", utils.FractalDir, err)
	}
	defer func() {
		cmd := exec.Command("chown", "-R", "ubuntu", utils.FractalDir)
		cmd.Run()
	}()

	// Create fractal-private directory
	err = os.MkdirAll(utils.FractalPrivateDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", utils.FractalPrivateDir, err)
	}

	// Create fractal temp directory
	err = os.MkdirAll(utils.TempDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Could not mkdir path %s. Error: %s", utils.TempDir, err)
	}
}

// Delete the directory used to store the mandelbox resource allocations (e.g.
// TTYs) on disk, as well as the directory used to store the SSL certificate we
// use for the httpserver, and our temporary directory.
func uninitializeFilesystem() {
	err := os.RemoveAll(utils.FractalDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.FractalDir, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", utils.FractalDir)
	}

	err = os.RemoveAll(utils.FractalPrivateDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.FractalPrivateDir, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", utils.FractalPrivateDir)
	}

	err = os.RemoveAll(utils.TempDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", utils.TempDir, err)
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
		// This function cleanly shuts down the Fractal Host Service. Note that
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

	initializeFilesystem(globalCancel)

	// Start Docker
	startDockerDaemon(globalCancel)
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

	// Start main event loop. Note that we don't track this goroutine, but
	// instead control its lifetime with `eventLoopKeepAlive`. This is because it
	// needs to stay alive after the global context is cancelled, so we can
	// process mandelbox death events.
	go eventLoopGoroutine(globalCtx, globalCancel, &goroutineTracker, dockerClient, httpServerEvents)

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

func eventLoopGoroutine(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, dockerClient *dockerclient.Client, httpServerEvents <-chan ServerRequest) {
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
				logger.Info("Got error \"%v\". Trying to start Docker daemon ourselves...", err)
				startDockerDaemon(globalCancel)
				continue
			default:
				if !strings.HasSuffix(strings.ToLower(err.Error()), "context canceled") {
					logger.Panicf(globalCancel, "Got an unknown error from the Docker event stream: %v", err)
				}
				return
			}

		case dockerevent := <-dockerevents:
			logger.Info("dockerevent: %s for %s %s\n", dockerevent.Action, dockerevent.Type, dockerevent.ID)
			if dockerevent.Action == "die" {
				mandelboxDieHandler(dockerevent.ID)
			}

		// It may seem silly to just launch goroutines to handle these
		// serverevents, but we aim to keep the high-level flow control and handling
		// in this package, and the low-level authentication, parsing, etc. of
		// requests in `httpserver`.
		case serverevent := <-httpServerEvents:
			switch serverevent.(type) {
			// TODO: actually handle panics in these goroutines
			case *SpinUpMandelboxRequest:
				go SpinUpMandelbox(globalCtx, globalCancel, goroutineTracker, dockerClient, serverevent.(*SpinUpMandelboxRequest))

			case *DrainAndShutdownRequest:
				// Don't do this in a separate goroutine, since there's no reason to.
				drainAndShutdown(globalCtx, globalCancel, goroutineTracker, serverevent.(*DrainAndShutdownRequest))

			default:
				if serverevent != nil {
					err := utils.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
					logger.Error(err)
					serverevent.ReturnResult("", err)
				}
			}
		}
	}
}
