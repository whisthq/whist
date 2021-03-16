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
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this impoted package as well.
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/cloudstorage"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	ecsagent "github.com/fractal/fractal/ecs-host-service/ecsagent"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer"
	webserver "github.com/fractal/fractal/ecs-host-service/fractalwebserver"
	httpserver "github.com/fractal/fractal/ecs-host-service/httpserver"

	dockertypes "github.com/docker/docker/api/types"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	dockerclient "github.com/docker/docker/client"
)

func init() {
	// Initialize random number generator for all subpackages
	rand.Seed(time.Now().UnixNano())
}

// The locations on disk where we store our data, including cloud storage
// mounts and container resource allocations. Note that we keep the cloud
// storage in its own directory, not in the fractalDir, so that we can safely
// delete the entire `fractal` directory on exit.
const fractalDir = "/fractal/"
const cloudStorageDir = "/fractalCloudStorage/"

// TODO: get rid of this security nemesis
// (https://github.com/fractal/fractal/issues/643)
// Opens all permissions on /fractal directory
func makeFractalDirectoriesFreeForAll() {
	cmd := exec.Command("chown", "-R", "ubuntu", fractalDir)
	cmd.Run()
	cmd = exec.Command("chmod", "-R", "777", fractalDir)
	cmd.Run()
	cmd = exec.Command("chown", "-R", "ubuntu", cloudStorageDir)
	cmd.Run()
	cmd = exec.Command("chmod", "-R", "777", cloudStorageDir)
	cmd.Run()
}

// Start the Docker daemon ourselves, to have control over all Docker containers spun
func startDockerDaemon() {
	cmd := exec.Command("/usr/bin/systemctl", "start", "docker")
	err := cmd.Run()
	if err != nil {
		logger.Panicf("Unable to start Docker daemon. Error: %v", err)
	} else {
		logger.Info("Successfully started the Docker daemon ourselves.")
	}
}

// We take ownership of the ECS agent ourselves
func startECSAgent() {
	go ecsagent.Main()
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

func handleCloudStorageRequest(req *httpserver.MountCloudStorageRequest) error {
	// Verify identifying hostPort value
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		return logger.MakeError("Invalid HostPort for cloud storage request: %v", req.HostPort)
	}
	hostPort := uint16(req.HostPort)

	fc, err := fractalcontainer.LookUpByIdentifyingHostPort(hostPort)
	if err != nil {
		return logger.MakeError("handleCloudStorageRequest(): %s", err)
	}

	provider, err := cloudstorage.GetProvider(req.Provider)
	if err != nil {
		return logger.MakeError("handleCloudStorageRequest(): Unable to parse provider: %s", err)
	}

	err = fc.AddCloudStorage(provider, req.AccessToken, req.RefreshToken, req.Expiry, req.TokenType, req.ClientID, req.ClientSecret)
	return err
}

// Handle tasks to be completed when a container dies
func containerDieHandler(ctx context.Context, cli *dockerclient.Client, id string) {
	// Exit if we are not dealing with a Fractal container.
	fc, err := fractalcontainer.LookUpByDockerID(fractalcontainer.DockerID(id))
	if err != nil {
		logger.Infof("containerDieHandler(): %s", err)
		return
	}

	err = fc.BackupUserConfigs()
	if err != nil {
		logger.Errorf("Couldn't back up user configs for container with FractalID %s: %s", fc.GetFractalID(), err)
	}

	fc.Close()
}

// ---------------------------
// Service shutdown and initialization
// ---------------------------

// Terminates the Fractal ECS host service
func shutdownHostService() {
	// Catch any panics in the calling goroutine. Note that besides the host
	// machine itself shutting down, this method should be the _only_ way that
	// this host service exits. In particular, we use panic() as a control flow
	// primitive --- panics in main(), its child functions, or any other calling
	// goroutines (such as the Ctrl+C signal handler) will be recovered here and
	// used as signals to exit. Therefore, panics should only be used	in the case
	// of an irrecoverable failure that mandates that the host machine accept no
	// new connections.
	r := recover()
	if r == nil {
		logger.Info("Beginning host service shutdown procedure.")
	} else {
		logger.Infof("Shutting down host service after caught panic: %v", r)
	}

	// Flush buffered logging events before the program terminates.
	logger.Info("Flushing Sentry...")
	logger.FlushSentry()
	logger.Info("Flushing Logzio...")
	logger.StopAndDrainLogzio()

	logger.Info("Sending final heartbeat...")
	webserver.SendGracefulShutdownNotice()

	logger.Info("Finished host service shutdown procedure. Finally exiting...")
	os.Exit(0)
}

// Create the directory used to store the container resource allocations
// (e.g. TTYs and cloud storage folders) on disk
func initializeFilesystem() {
	// check if "/fractal" already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(fractalDir); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf("Directory %s already exists!", fractalDir)
		} else {
			logger.Panicf("Could not make directory %s because of error %v", fractalDir, err)
		}
	}

	// Create the resource mapping directory
	err := os.MkdirAll(fractalDir, 0777)
	if err != nil {
		logger.Panicf("Failed to create directory %s: error: %s\n", fractalDir, err)
	}

	// Same check as above, but for fractal-private directory
	if _, err := os.Lstat(httpserver.FractalPrivatePath); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf("Directory %s already exists!", httpserver.FractalPrivatePath)
		} else {
			logger.Panicf("Could not make directory %s because of error %v", httpserver.FractalPrivatePath, err)
		}
	}

	// Create fractal-private directory
	err = os.MkdirAll(httpserver.FractalPrivatePath, 0777)
	if err != nil {
		logger.Panicf("Failed to create directory %s: error: %s\n", httpserver.FractalPrivatePath, err)
	}

	// Don't check for cloud storage directory, since that makes
	// testing/debugging a lot easier and safer (we don't want to delete the
	// directory on exit, since that might delete files from people's cloud
	// storage drives.)

	// Create cloud storage directory
	err = os.MkdirAll(cloudStorageDir, 0777)
	if err != nil {
		logger.Panicf("Could not mkdir path %s. Error: %s", cloudStorageDir, err)
	}

	// Create fractal temp directory
	err = os.MkdirAll("/fractal/temp/", 0777)
	if err != nil {
		logger.Panicf("Could not mkdir path %s. Error: %s", cloudStorageDir, err)
	}

	makeFractalDirectoriesFreeForAll()
}

// Delete the directory used to store the container resource allocations (e.g.
// TTYs and cloud storage folders) on disk, as well as the directory used to
// store the SSL certificate we use for the httpserver.
func uninitializeFilesystem() {
	err := os.RemoveAll(fractalDir)
	if err != nil {
		logger.Panicf("Failed to delete directory %s: error: %v\n", fractalDir, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", fractalDir)
	}

	err = os.RemoveAll(httpserver.FractalPrivatePath)
	if err != nil {
		logger.Panicf("Failed to delete directory %s: error: %v\n", httpserver.FractalPrivatePath, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", httpserver.FractalPrivatePath)
	}

	// TODO: Unmount all cloud-storage folders
}

func main() {
	// The host service needs root permissions.
	logger.RequireRootPermissions()

	// After the permissions check, this needs to be the first statement in
	// main(). This deferred function allows us to catch any panics in the main
	// goroutine and therefore execute code on shutdown of the host service. In
	// particular, we want to send a message to Sentry and/or the fractal
	// webserver upon our death.
	defer shutdownHostService()

	rand.Seed(time.Now().UnixNano())

	// Note that we defer uninitialization so that in case of panic elsewhere, we
	// still clean up
	initializeFilesystem()
	defer uninitializeFilesystem()

	// Register a signal handler for Ctrl-C so that we cleanup if Ctrl-C is pressed.
	sigChan := make(chan os.Signal, 2)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	go func() {
		defer shutdownHostService()
		<-sigChan
		logger.Info("Got an interrupt or SIGTERM --- calling uninitializeFilesystem() and initiating host shutdown process...")
		uninitializeFilesystem()
	}()

	// Log the Git commit of the running executable
	logger.Info("Host Service Version: %s", logger.GetGitCommit())

	// Initialize webserver heartbeat
	err := webserver.InitializeHeartbeat()
	if err != nil {
		logger.Panicf("Unable to initialize webserver heartbeats. Error: %s", err)
	}

	// Start the HTTP server and listen for events
	httpServerEvents, err := httpserver.StartHTTPSServer()
	if err != nil {
		logger.Panic(err)
	}

	// Start Docker Daemon and ECS Agent. Notably, this needs to happen after the
	// webserver handshake above. This prevents AWS from assigning any task
	// definitions to our container before the webserver knows about it.
	startDockerDaemon()
	// Only start the ECS Agent if we are talking to a dev, staging, or
	// production webserver.
	if logger.GetAppEnvironment() != logger.EnvLocalDev {
		logger.Infof("Talking to the %v webserver -- starting ECS Agent.", logger.GetAppEnvironment())
		startECSAgent()
	} else {
		logger.Infof("Running in environment LocalDev, so not starting ecs-agent.")
	}

	ctx := context.Background()
	cli, err := dockerclient.NewClientWithOpts(dockerclient.FromEnv)
	if err != nil {
		logger.Panicf("Error creating new Docker client: %v", err)
	}

	filters := dockerfilters.NewArgs()
	filters.Add("type", dockerevents.ContainerEventType)
	eventOptions := dockertypes.EventsOptions{
		Filters: filters,
	}

	// In the following loop, this var determines whether to re-initialize the
	// Docker event stream. This is necessary because the Docker event stream
	// needs to be reopened after any error is sent over the error channel.
	needToReinitDockerEventStream := false
	dockerevents, dockererrs := cli.Events(context.Background(), eventOptions)
	logger.Info("Initialized docker event stream.")
	logger.Info("Entering event loop...")

eventLoop:
	for {
		if needToReinitDockerEventStream {
			dockerevents, dockererrs = cli.Events(context.Background(), eventOptions)
			needToReinitDockerEventStream = false
			logger.Info("Re-initialized docker event stream.")
		}

		select {
		case err := <-dockererrs:
			needToReinitDockerEventStream = true
			switch {
			case err == nil:
				logger.Info("We got a nil error over the Docker event stream. This might indicate an error inside the docker go library. Ignoring it and proceeding normally...")
				continue
			case err == io.EOF:
				logger.Panicf("Docker event stream has been completely read.")
				break eventLoop
			case dockerclient.IsErrConnectionFailed(err):
				// This means "Cannot connect to the Docker daemon..."
				logger.Info("Got error \"%v\". Trying to start Docker daemon ourselves...", err)
				startDockerDaemon()
				continue
			default:
				logger.Panicf("Got an unknown error from the Docker event stream: %v", err)
			}

		case dockerevent := <-dockerevents:
			if dockerevent.Action == "die" || dockerevent.Action == "start" {
				logger.Info("dockerevent: %s for %s %s\n", dockerevent.Action, dockerevent.Type, dockerevent.ID)
			}
			if dockerevent.Action == "die" {
				// Since we want all steps in the die handler to be attempted,
				// regardless of earlier errors, we let the containerDieHandler report
				// its own errors, and return nothing to us.
				containerDieHandler(ctx, cli, dockerevent.ID)
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
				err := handleCloudStorageRequest(serverevent.(*httpserver.MountCloudStorageRequest))
				if err != nil {
					logger.Error(err)
				}
				serverevent.ReturnResult("", err)

			default:
				err := logger.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
				serverevent.ReturnResult("", err)
				logger.Error(err)
			}
		}
	}
}
