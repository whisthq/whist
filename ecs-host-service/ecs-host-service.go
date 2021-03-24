package main

import (
	"math"

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

// Creates a file containing the DPI assigned to a specific container, and make
// it accessible to that container. Also take the received User ID and retrieve
// the user's app configs if the User ID is set.
// Takes the request to the `set_container_start_values` endpoint as
// an argument and returns nil if no errors, and error object if error.
func handleStartValuesRequest(req *httpserver.SetContainerStartValuesRequest) error {
	// Verify identifying hostPort value
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		return logger.MakeError("Invalid HostPort for start values request: %v", req.HostPort)
	}
	hostPort := uint16(req.HostPort)

	fc, err := fractalcontainer.LookUpByIdentifyingHostPort(hostPort)
	if err != nil {
		return logger.MakeError("handleStartValuesRequest(): %s", err)
	}

	fc.AssignToUser(fractalcontainer.UserID(req.UserID))

	err = fc.WriteStartValues(req.DPI, req.ContainerARN)
	if err != nil {
		return logger.MakeError("handleStartValuesRequest(): %s", err)
	}

	// Populate the user config folder for the container's app
	err = fc.PopulateUserConfigs()
	if err != nil {
		return logger.MakeError("handleStartValuesRequest(): %s", err)
	}

	err = fc.MarkReady()
	if err != nil {
		return logger.MakeError("handleStartValuesRequest(): %s", err)
	}

	return nil
}

// We make this function return its own error, so that we don't block the event
// loop  goroutine with `rclone mount` commands (which are fast in the happy
// path, but can take a minute in the worst case).
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
		logAndReturnError("handleCloudStorageRequest(): %s", err)
		return
	}

	provider, err := cloudstorage.GetProvider(req.Provider)
	if err != nil {
		logAndReturnError("handleCloudStorageRequest(): Unable to parse provider: %s", err)
		return
	}

	// We add the cloud storage in a separate goroutine, since mounting (even in
	// background/daemon mode) can take over a minute.
	errChan := make(chan error)
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		errChan <- fc.AddCloudStorage(goroutineTracker, provider, req.AccessToken, req.RefreshToken, req.Expiry, req.TokenType, req.ClientID, req.ClientSecret)
	}()

	logAndReturnError((<-errChan).Error())
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
					// Since we want all steps in the die handler to be attempted,
					// regardless of earlier errors, we let the containerDieHandler report
					// its own errors, and return nothing to us.
					containerDieHandler(dockerevent.ID)
				}

			case serverevent := <-httpServerEvents:
				switch serverevent.(type) {
				case *httpserver.SetContainerStartValuesRequest:
					err := handleStartValuesRequest(serverevent.(*httpserver.SetContainerStartValuesRequest))
					if err != nil {
						logger.Error(err)
					}
					serverevent.ReturnResult("", err)

				case *httpserver.MountCloudStorageRequest:
					handleCloudStorageRequest(globalCtx, globalCancel, goroutineTracker, serverevent.(*httpserver.MountCloudStorageRequest))

				default:
					err := logger.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
					serverevent.ReturnResult("", err)
					logger.Error(err)
				}

			}
		}
	}()
}
