/*
The Whist Host-Service is responsible for orchestrating mandelboxes (i.e.
Whist-enabled containers) on EC2 instances (referred to as "hosts" throughout
the codebase). The host-service is responsible for making Docker calls to start
and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on
the same host (by dynamically allocating and assigning resources), and for
passing startup data to the mandelboxes, both from the rest of the backend and
from the user's frontend application.

If you are just interested in seeing what endpoints the host-service exposes
(i.e. for frontend development), check out the file `httpserver.go`.

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
	"path"
	"regexp"
	"strings"
	"sync"
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry. For the same reason, we make sure not
	// to import the fmt package either, instead separating required
	// functionality in this imported package as well.

	"github.com/whisthq/whist/backend/services/httputils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	mandelboxData "github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"

	dockertypes "github.com/docker/docker/api/types"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	dockerclient "github.com/docker/docker/client"
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
		// haven't even entered the host-service main() yet.
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

// Given a list of regexes, find a Docker image whose name matches the earliest
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

// Drain and shutdown the host-service
func drainAndShutdown(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	logger.Infof("Got a DrainAndShutdownRequest... cancelling the global context.")
	shutdownInstanceOnExit = true
	globalCancel()
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

	// Gracefully shut down the mandelbox Docker container
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
	// Check if the instance has ephemeral storage. If it does, set the WhistDir path to the
	// ephemeral volume, which should already be mounted by the userdata script. We use the
	// command below to check if the instance has an ephemeral device present.
	// See: https://stackoverflow.com/questions/10781516/how-to-pipe-several-commands-in-go
	ephemeralDeviceCmd := "nvme list -o json | jq -r '.Devices | map(select(.ModelNumber == \"Amazon EC2 NVMe Instance Storage\")) | max_by(.PhysicalSize) | .DevicePath'"
	out, err := exec.Command("bash", "-c", ephemeralDeviceCmd).CombinedOutput()
	if err != nil {
		logger.Errorf("Error while getting ephemeral device path, not using ephemeral storage.")
	}

	ephemeralDevicePath := string(out)

	// We check if the command exited successfully, and if the ephemeral device exists.
	// The string will contain "bash" if something went wrong. Also take into account
	// if the WhistDir already contains the ephemeral path.
	if !metadata.IsLocalEnv() &&
		ephemeralDevicePath != "" && ephemeralDevicePath != "null" &&
		!strings.Contains(ephemeralDevicePath, "bash") && !strings.Contains(utils.WhistDir, utils.WhistEphemeralFSPath) {
		logger.Infof("Creating Whist directory on ephemeral device.")
		utils.WhistDir = path.Join(utils.WhistEphemeralFSPath, utils.WhistDir)
		utils.TempDir = path.Join(utils.WhistDir, "temp/")
	}

	// check if "/whist" already exists --- if so, panic, since
	// we don't know why it's there or if it's valid. The host-service shutting down
	// from this panic will clean up the directory and the next run will work properly.
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
	err = os.MkdirAll(utils.WhistDir, 0777)
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
	// The first thing we want to do is to initialize Logz.io and Sentry so that
	// we can catch any errors that might occur, or logs if we print them.
	logger.InitHostLogging()

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
		// This function cleanly shuts down the Whist Host-Service. Note that
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

	if err := dbdriver.RegisterInstance(); err != nil {
		// If the instance starts up and sees its status as unresponsive or
		// draining, the backend doesn't want it anymore so we should shut down.

		// TODO: make this a bit more robust
		if !metadata.IsLocalEnv() && strings.Contains(err.Error(), string(dbdriver.InstanceStatusDraining)) {
			logger.Infof("Instance wasn't registered in database because we found ourselves already marked draining. Shutting down.... Error: %s", err)
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
	instanceID, err := aws.GetInstanceID()
	if err != nil {
		logger.Errorf("Can't get AWS Instance Name to start database subscriptions. Error: %s", err)
		metrics.Increment("ErrorRate")
	}
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)
	// It's not necessary to subscribe to the config database
	// in the host service
	useConfigDB := false

	subscriptionClient := &subscriptions.SubscriptionClient{}
	subscriptions.SetupHostSubscriptions(string(instanceID), subscriptionClient)
	subscriptions.Start(subscriptionClient, globalCtx, &goroutineTracker, subscriptionEvents, useConfigDB)
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
	httpServerEvents <-chan httputils.ServerRequest, subscriptionEvents <-chan subscriptions.SubscriptionEvent) {
	// Note that we don't use globalCtx for the Docker Context, since we still
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

					instanceID, err := aws.GetInstanceID()
					if err != nil {
						logger.Errorf("Error getting instance name from AWS, %v", err)
						metrics.Increment("ErrorRate")
					}
					// Create a mandelbox object as would be received from a Hasura subscription.
					mandelbox := subscriptions.Mandelbox{
						InstanceID: string(instanceID),
						ID:         jsonReq.MandelboxID,
						SessionID:  "1234",
						UserID:     userID,
					}
					subscriptionEvent := subscriptions.MandelboxEvent{
						Mandelboxes: []subscriptions.Mandelbox{mandelbox},
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
