package main

import (
	"math"
	"strings"

	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via Sentry. Instead, use the
	// fractallogger package imported below as `logger`
	"context"
	"io"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"sync"
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this imported package as well.
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	"github.com/fractal/fractal/ecs-host-service/ecsagent"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"
	"github.com/fractal/fractal/ecs-host-service/httpserver"

	dockertypes "github.com/docker/docker/api/types"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	dockerclient "github.com/docker/docker/client"
)

func init() {
	// Initialize random number generator for all subpackages
	rand.Seed(time.Now().UnixNano())
}

// Start the Docker daemon ourselves, to have control over all Docker containers spun
func startDockerDaemon(globalCancel context.CancelFunc) {
	cmd := exec.Command("/usr/bin/systemctl", "start", "docker")
	err := cmd.Run()
	if err != nil {
		logger.Panicf(globalCancel, "Unable to start Docker daemon. Error: %v", err)
	} else {
		logger.Info("Successfully started the Docker daemon ourselves.")
	}
}

// We take ownership of the ECS agent ourselves
func startECSAgent(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()
		exitCode := ecsagent.Main(globalCtx, globalCancel, goroutineTracker)

		// If we got here, then that means that the ecsagent has exited for some
		// reason (that means the context we passed in was cancelled, or there was
		// some initialization error). Regardless, we "panic" and cancel the context.
		logger.Panicf(globalCancel, "ECS Agent exited with code %d", exitCode)
	}()
}

// ------------------------------------
// Container event handlers
// ------------------------------------

// This function will only be called in the localdev environment. It is also
// currently only used in `run_container_image.sh`. Eventually, as we move off
// ECS, this endpoint will become the canonical way to start containers.
func SpinUpContainer(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, req *httpserver.SpinUpContainerRequest) {
	logAndReturnError := func(fmt string, v ...interface{}) {
		err := logger.MakeError("handleStartValuesRequest(): "+fmt, v...)
		logger.Error(err)
		req.ReturnResult("", err)
	}

	fc := fractalcontainer.New(globalCtx, goroutineTracker, "abcdefabcdefabcdefabcdefabcdef")
	logger.Infof("SpinUpContainer(): created FractalContainer object %s", fc.GetFractalID())

	// We just fix the ports we want
	if err := fc.AssignPortBindings([]portbindings.PortBinding{
		{ContainerPort: 32262, HostPort: 32262, BindIP: "", Protocol: "tcp"},
		{ContainerPort: 32263, HostPort: 32263, BindIP: "", Protocol: "udp"},
		{ContainerPort: 32273, HostPort: 32273, BindIP: "", Protocol: "tcp"},
	}); err != nil {
		logAndReturnError("SpinUpContainer(): error assigning port bindings: %s", err)
		return
	}
	logger.Infof("SpinUpContainer(): successfully assigned port bindings.")
	portBindings := fc.GetPortBindings()

	if err := fc.InitializeUinputDevices(goroutineTracker); err != nil {
		logAndReturnError("SpinUpContainer(): error initializing uinput devices: %s", err)
		return
	}
	logger.Infof("SpinUpContainer(): successfully initialized uinput devices.")
	devices := fc.GetDeviceMappings()

	if err := fc.InitializeTTY(); err != nil {
		logAndReturnError("SpinUpContainer(): error initializing TTY: %s", err)
		return
	}
	logger.Infof("SpinUpContainer(): successfully initialized TTY.")

	// Get AppName, appImage, and mountCommand from environment variables (set by run_container_image.sh)
	appName := fractalcontainer.AppName(req.AppName)

	logger.Infof(`SpinUpContainer(): app name: "%s"`, req.AppName)
	logger.Infof(`SpinUpContainer(): app image: "%s"`, req.AppImage)
	logger.Infof(`SpinUpContainer(): mount command: "%s"`, req.MountCommand)

	// Actually create the container using `docker create`. Yes, this could be
	// done with the docker CLI, but that would take some effort to convert.
	// This was "quick" and dirty. Also, once we're off ECS we'll be able to
	// test running containers on any Linux machine without hackery like this,
	// so this is temporary in any case.

	dockerCmdArgs := []string{
		"create", "-it",
		`-v`, `/sys/fs/cgroup:/sys/fs/cgroup:ro`,
		`-v`, `/fractal/` + string(fc.GetFractalID()) + `/containerResourceMappings:/fractal/resourceMappings:ro`,
		`-v`, `/fractalCloudStorage/` + string(fc.GetFractalID()) + `:/fractal/cloudStorage:rshared`,
		`-v`, `/fractal/temp/` + string(fc.GetFractalID()) + `/sockets:/tmp/sockets`,
		`-v`, `/run/udev/data:/run/udev/data:ro`,
		`-v`, `/fractal/` + string(fc.GetFractalID()) + `/userConfigs:/fractal/userConfigs:rshared`,
		logger.Sprintf(`--device=%s:%s:%s`, devices[0].PathOnHost, devices[0].PathInContainer, devices[0].CgroupPermissions),
		logger.Sprintf(`--device=%s:%s:%s`, devices[1].PathOnHost, devices[1].PathInContainer, devices[1].CgroupPermissions),
		logger.Sprintf(`--device=%s:%s:%s`, devices[2].PathOnHost, devices[2].PathInContainer, devices[2].CgroupPermissions),
	}
	if req.MountCommand != "" {
		dockerCmdArgs = append(dockerCmdArgs, strings.Split(req.MountCommand, " ")...)
	}
	dockerCmdArgs = append(dockerCmdArgs, []string{
		`--tmpfs`, `/run`,
		`--tmpfs`, `/run/lock`,
		`--gpus`, `all`,
		`-e`, `NVIDIA_CONTAINER_CAPABILITIES=all`,
		`-e`, `NVIDIA_VISIBLE_DEVICES=all`,
		`--shm-size=8g`,
		`--cap-drop`, `ALL`,
		`--cap-add`, `CAP_SETPCAP`,
		`--cap-add`, `CAP_MKNOD`,
		`--cap-add`, `CAP_AUDIT_WRITE`,
		`--cap-add`, `CAP_CHOWN`,
		`--cap-add`, `CAP_NET_RAW`,
		`--cap-add`, `CAP_DAC_OVERRIDE`,
		`--cap-add`, `CAP_FOWNER`,
		`--cap-add`, `CAP_FSETID`,
		`--cap-add`, `CAP_KILL`,
		`--cap-add`, `CAP_SETGID`,
		`--cap-add`, `CAP_SETUID`,
		`--cap-add`, `CAP_NET_BIND_SERVICE`,
		`--cap-add`, `CAP_SYS_CHROOT`,
		`--cap-add`, `CAP_SETFCAP`,
		// NOTE THAT CAP_NICE IS NOT ENABLED BY DEFAULT BY DOCKER --- THIS IS OUR DOING
		`--cap-add`, `SYS_NICE`,
		`-p`, logger.Sprintf(`%v:%v/%s`, portBindings[0].HostPort, portBindings[0].ContainerPort, portBindings[0].Protocol),
		`-p`, logger.Sprintf(`%v:%v/%s`, portBindings[1].HostPort, portBindings[1].ContainerPort, portBindings[1].Protocol),
		`-p`, logger.Sprintf(`%v:%v/%s`, portBindings[2].HostPort, portBindings[2].ContainerPort, portBindings[2].Protocol),
		req.AppImage,
	}...)

	cmd := exec.CommandContext(globalCtx, "docker", dockerCmdArgs...)
	logger.Infof("SpinUpContainer(): Running `docker create` command:\n%s\n\n", cmd)
	output, err := cmd.CombinedOutput()
	if err != nil {
		logAndReturnError("SpinUpContainer(): Error running `docker create` for %s:\n%s", fc.GetFractalID(), output)
		return
	}
	dockerID := fractalcontainer.DockerID(strings.TrimSpace(string(output)))
	logger.Infof("SpinUpContainer(): Successfully ran `docker create` command and got back DockerID %s", dockerID)

	err = fc.RegisterCreation(dockerID, appName)
	if err != nil {
		logAndReturnError("SpinUpContainer(): error registering container creation with DockerID %s and AppName %s: %s", dockerID, req.AppName, err)
		return
	}
	logger.Infof("SpinUpContainer(): Successfully registered container creation with DockerID %s and AppName %s", dockerID, appName)

	cmd = exec.CommandContext(globalCtx, "docker", "start", string(dockerID))
	logger.Infof("SpinUpContainer(): Running `docker start` command.")
	output, err = cmd.CombinedOutput()
	if err != nil {
		logAndReturnError("SpinUpContainer(): Error running `docker start`: %s", output)
		return
	}
	logger.Infof("SpinUpContainer(): Successfully started container.")

	if err := fc.WriteResourcesForProtocol(); err != nil {
		logAndReturnError("SpinUpContainer(): error writing resources for protocol: %s", err)
		return
	}
	logger.Infof("SpinUpContainer(): Successfully wrote resources for protocol.")

	// Return dockerID of newly created container
	req.ReturnResult(string(dockerID), nil)
	logger.Infof("SpinUpContainer(): Finished starting up container %s", fc.GetFractalID())
}

// Creates a file containing the DPI assigned to a specific container, and make
// it accessible to that container. Also take the received User ID and retrieve
// the user's app configs if the User ID is set. We make this function send
// back the result for the provided request so that we can run in its own
// goroutine and not block the event loop goroutine.
func handleStartValuesRequest(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, req *httpserver.SetContainerStartValuesRequest) {
	logAndReturnError := func(fmt string, v ...interface{}) {
		err := logger.MakeError("handleStartValuesRequest(): "+fmt, v...)
		logger.Error(err)
		req.ReturnResult("", err)
	}

	// Verify identifying hostPort value
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		logAndReturnError("Invalid HostPort: %v", req.HostPort)
		return
	}
	hostPort := uint16(req.HostPort)

	fc, err := fractalcontainer.LookUpByIdentifyingHostPort(hostPort)
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	fc.AssignToUser(fractalcontainer.UserID(req.UserID))

	err = fc.WriteStartValues(req.DPI, req.ContainerARN)
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	// Populate the user config folder for the container's app
	err = fc.PopulateUserConfigs()
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	err = fc.MarkReady()
	if err != nil {
		logAndReturnError(err.Error())
		return
	}

	req.ReturnResult("", nil)
}

// We make this function send back the result for the provided request so that
// we can run in its own goroutine and not block the event loop goroutine with
// `rclone mount` commands (which are fast in the happy path, but can take a
// minute in the worst case).
func handleCloudStorageRequest(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, req *httpserver.MountCloudStorageRequest) {
	logAndReturnError := func(fmt string, v ...interface{}) {
		err := logger.MakeError("handleCloudStorageRequest(): "+fmt, v...)
		logger.Error(err)
		req.ReturnResult("", err)
	}

	// Verify identifying hostPort value
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		logAndReturnError("Invalid HostPort: %v", req.HostPort)
		return
	}
	hostPort := uint16(req.HostPort)

	fc, err := fractalcontainer.LookUpByIdentifyingHostPort(hostPort)
	if err != nil {
		logAndReturnError("%s", err)
		return
	}

	provider, err := cloudstorage.GetProvider(req.Provider)
	if err != nil {
		logAndReturnError("Unable to parse provider: %s", err)
		return
	}

	err = fc.AddCloudStorage(goroutineTracker, provider, req.AccessToken, req.RefreshToken, req.Expiry, req.TokenType, req.ClientID, req.ClientSecret)
	if err != nil {
		logAndReturnError(err.Error())
	}

	req.ReturnResult("", nil)
}

// Handle tasks to be completed when a container dies
func containerDieHandler(id string) {
	// Exit if we are not dealing with a Fractal container, or if it has already
	// been closed (via a call to Close() or a context cancellation).
	fc, err := fractalcontainer.LookUpByDockerID(fractalcontainer.DockerID(id))
	if err != nil {
		logger.Infof("containerDieHandler(): %s", err)
		return
	}

	fc.Close()
}

// ---------------------------
// Service shutdown and initialization
// ---------------------------

// Create the directory used to store the container resource allocations
// (e.g. TTYs and cloud storage folders) on disk
func initializeFilesystem(globalCancel context.CancelFunc) {
	// check if "/fractal" already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(logger.FractalDir); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf(globalCancel, "Directory %s already exists!", logger.FractalDir)
		} else {
			logger.Panicf(globalCancel, "Could not make directory %s because of error %v", logger.FractalDir, err)
		}
	}

	// Create the fractal directory and make it non-root user owned so that
	// non-root users in containers can access files within (especially user
	// configs). We do this in a deferred function so that any subdirectories
	// created later in this function are also covered.
	err := os.MkdirAll(logger.FractalDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", logger.FractalDir, err)
	}
	defer func() {
		cmd := exec.Command("chown", "-R", "ubuntu", logger.FractalDir)
		cmd.Run()
	}()

	// Create fractal-private directory
	err = os.MkdirAll(httpserver.FractalPrivatePath, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Failed to create directory %s: error: %s\n", httpserver.FractalPrivatePath, err)
	}

	// Create cloud storage directory and make it owned by a non-root user so
	// it's accessible by a non-root user inside the container. Notably, we also
	// disable the executable bit for this directory.
	err = os.MkdirAll(fractalcontainer.FractalCloudStorageDir, 0666)
	if err != nil {
		logger.Panicf(globalCancel, "Could not mkdir path %s. Error: %s", fractalcontainer.FractalCloudStorageDir, err)
	}
	cmd := exec.Command("chown", "-R", "ubuntu", fractalcontainer.FractalCloudStorageDir)
	cmd.Run()

	// Create fractal temp directory
	err = os.MkdirAll(logger.TempDir, 0777)
	if err != nil {
		logger.Panicf(globalCancel, "Could not mkdir path %s. Error: %s", logger.TempDir, err)
	}
}

// Delete the directory used to store the container resource allocations (e.g.
// TTYs and cloud storage folders) on disk, as well as the directory used to
// store the SSL certificate we use for the httpserver, and our temporary
// directory.
func uninitializeFilesystem() {
	err := os.RemoveAll(logger.FractalDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", logger.FractalDir, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", logger.FractalDir)
	}

	err = os.RemoveAll(httpserver.FractalPrivatePath)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", httpserver.FractalPrivatePath, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", httpserver.FractalPrivatePath)
	}

	err = os.RemoveAll(logger.TempDir)
	if err != nil {
		logger.Errorf("Failed to delete directory %s: error: %v\n", logger.TempDir, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", logger.TempDir)
	}

	// We don't want to delete the cloud storage directory on exit, since that
	// might delete files from people's cloud storage drives.
}

func main() {
	// The host service needs root permissions.
	logger.RequireRootPermissions()

	// We create a global context (i.e. for the entire host service) that can be
	// cancelled if the entire program needs to terminate. We also create a
	// WaitGroup for all goroutines to tell us when they've stopped (if the
	// context gets cancelled). Finally, we defer a function which
	// cancels the global context if necessary, logs any panic we might be
	// recovering from, and cleans up after the entire host service. After
	// the permissions check, the creation of this context and WaitGroup, and the
	// following defer must be the first statements in main().
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}
	defer func() {
		// This function cleanly shuts down the Fractal ECS host service. Note that
		// besides the host machine itself shutting down, this deferred function
		// from main() should be the _only_ way that the host service exits. In
		// particular, it should be as a result of a panic() in main, the global
		// context being cancelled, or a Ctrl+C interrupt.

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

		// Cancel the global context, if it hasn't already been cancelled. Note
		// that this also cleans up after every container.
		globalCancel()

		// Wait for all goroutines to stop, so we can run the rest of the cleanup
		// process.
		goroutineTracker.Wait()

		// Shut down the logging infrastructure.
		logger.Close()

		uninitializeFilesystem()

		logger.Info("Finished host service shutdown procedure. Finally exiting...")
		os.Exit(0)
	}()

	// Log the Git commit of the running executable
	logger.Info("Host Service Version: %s", logger.GetGitCommit())

	initializeFilesystem(globalCancel)

	// Now we start all the goroutines that actually do work.

	// Start the HTTP server and listen for events
	httpServerEvents, err := httpserver.Start(globalCtx, globalCancel, &goroutineTracker)
	if err != nil {
		logger.Panic(globalCancel, err)
	}

	startDockerDaemon(globalCancel)

	// Only start the ECS Agent if we are talking to a dev, staging, or
	// production webserver.
	if logger.GetAppEnvironment() != logger.EnvLocalDev {
		logger.Infof("Talking to the %v webserver -- starting ECS Agent.", logger.GetAppEnvironment())
		startECSAgent(globalCtx, globalCancel, &goroutineTracker)
	} else {
		logger.Infof("Running in environment LocalDev, so not starting ecs-agent.")
	}

	// Start main event loop
	startEventLoop(globalCtx, globalCancel, &goroutineTracker, httpServerEvents)

	// Register a signal handler for Ctrl-C so that we cleanup if Ctrl-C is pressed.
	sigChan := make(chan os.Signal, 2)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	// Wait for either the global context to get cancelled by a worker goroutine,
	// or for us to receive an interrupt. This needs to be the end of main().
	select {
	case <-sigChan:
		logger.Infof("Got an interrupt or SIGTERM")
	case <-globalCtx.Done():
		logger.Errorf("Global context cancelled!")
	}
}

func startEventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, httpServerEvents <-chan httpserver.ServerRequest) {
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Create docker client
		cli, err := dockerclient.NewClientWithOpts(dockerclient.FromEnv)
		if err != nil {
			logger.Panicf(globalCancel, "Error creating new Docker client: %v", err)
		}

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
		dockerevents, dockererrs := cli.Events(globalCtx, eventOptions)
		logger.Info("Initialized docker event stream.")
		logger.Info("Entering event loop...")

		// The actual event loop
		for {
			if needToReinitDockerEventStream {
				dockerevents, dockererrs = cli.Events(globalCtx, eventOptions)
				needToReinitDockerEventStream = false
				logger.Info("Re-initialized docker event stream.")
			}

			select {
			case <-globalCtx.Done():
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
					logger.Panicf(globalCancel, "Got an unknown error from the Docker event stream: %v", err)
				}

			case dockerevent := <-dockerevents:
				if dockerevent.Action == "die" || dockerevent.Action == "start" {
					logger.Info("dockerevent: %s for %s %s\n", dockerevent.Action, dockerevent.Type, dockerevent.ID)
				}
				if dockerevent.Action == "die" {
					containerDieHandler(dockerevent.ID)
				}

			// It may seem silly to just launch goroutines to handle these
			// serverevents, but keeping the high-level flow control and handling in
			// this package, and the low-level authentication, parsing, etc. of
			// requests in `httpserver` provides code quality benefits.
			case serverevent := <-httpServerEvents:
				switch serverevent.(type) {
				case *httpserver.SetContainerStartValuesRequest:
					go handleStartValuesRequest(globalCtx, globalCancel, goroutineTracker, serverevent.(*httpserver.SetContainerStartValuesRequest))

				case *httpserver.MountCloudStorageRequest:
					go handleCloudStorageRequest(globalCtx, globalCancel, goroutineTracker, serverevent.(*httpserver.MountCloudStorageRequest))

				case *httpserver.SpinUpContainerRequest:
					go SpinUpContainer(globalCtx, globalCancel, goroutineTracker, serverevent.(*httpserver.SpinUpContainerRequest))

				default:
					if serverevent != nil {
						err := logger.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
						logger.Error(err)
						serverevent.ReturnResult("", err)
					}
				}

			}
		}
	}()
}
