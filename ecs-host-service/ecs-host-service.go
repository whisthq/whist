package main

import (
	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via sentry. Instead, use the
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
	// forget to send a message via sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this impoted package as well.
	logger "github.com/fractalcomputers/ecs-host-service/fractallogger"

	webserver "github.com/fractalcomputers/ecs-host-service/fractalwebserver"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/events"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/client"
)

// The location on disk where we store the container resource allocations
const resourceMappingDirectory = "/fractal/containerResourceMappings/"

// Check that the program has been started with the correct permissions --- for
// now, we just want to run as root, but this service could be assigned its own
// user in the future
func checkRunningPermissions() {
	if os.Geteuid() != 0 {
		logger.Panicf("This service needs to run as root!")
	}
}

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
	cmd := exec.Command("/usr/bin/systemctl", "enable", "docker-container@ecs-agent")
	err := cmd.Run()
	if err != nil {
		logger.Panicf("Unable to start ECS-agent. Error: %v", err)
	} else {
		logger.Info("Successfully started the ECS agent ourselves.")
	}
}

func shutdownHostService() {
	logger.Info("Beginning host service shutdown procedure.")

	// Catch any panics in the calling goroutine. Note that besides the host
	// machine itself shutting down, this method should be the _only_ way that
	// this host service exits. In particular, we use panic() as a control flow
	// primitive --- panics in main(), its child functions, or any other calling
	// goroutines (such as the Ctrl+C signal handler) will be recovered here and
	// used as signals to exit. Therefore, panics should only be used	in the case
	// of an irrecoverable failure that mandates that the host machine accept no
	// new connections.
	r := recover()
	logger.Errorf("shutdownHostService(): Caught panic: %v", r)
	logger.PrintStackTrace()

	// Flush buffered Sentry events before the program terminates.
	logger.Info("Flushing sentry...")
	logger.FlushSentry()

	logger.Info("Sending final heartbeat...")
	webserver.SendGracefulShutdownNotice()

	logger.Info("Finished host service shutdown procedure. Finally exiting...")
	os.Exit(0)
}

// Create the directory used to store the container resource allocations (e.g.
// TTYs) on disk
func initializeFilesystem() {
	// check if resource mapping directory already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(resourceMappingDirectory); !os.IsNotExist(err) {
		if err == nil {
			logger.Panicf("Directory %s already exists!", resourceMappingDirectory)
		} else {
			logger.Panicf("Could not make directory %s because of error %v", resourceMappingDirectory, err)
		}
	}

	err := os.MkdirAll(resourceMappingDirectory, 0644|os.ModeSticky)
	if err != nil {
		logger.Panicf("Failed to create directory %s: error: %s\n", resourceMappingDirectory, err)
	}
}

func uninitializeFilesystem() {
	err := os.RemoveAll(resourceMappingDirectory)
	if err != nil {
		logger.Panicf("Failed to delete directory %s: error: %v\n", resourceMappingDirectory, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", resourceMappingDirectory)
	}
}

func writeAssignmentToFile(filename, data string) (err error) {
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644|os.ModeSticky)
	if err != nil {
		return logger.MakeError("Unable to create file %s to store resource assignment. Error: %v", filename, err)
	}
	// Instead of deferring the close() and sync() of the file, as is
	// conventional, we do it at the end of the function to avoid some annoying
	// linter errors
	_, err = file.WriteString(data)
	if err != nil {
		return logger.MakeError("Couldn't write assignment with data %s to file %s. Error: %v", data, filename, err)
	}

	err = file.Sync()
	if err != nil {
		return logger.MakeError("Couldn't sync file %s. Error: %v", filename, err)
	}

	err = file.Close()
	if err != nil {
		return logger.MakeError("Couldn't close file %s. Error: %v", filename, err)
	}

	logger.Info("Wrote data \"%s\" to file %s\n", data, filename)
	return nil
}

func containerStartHandler(ctx context.Context, cli *client.Client, id string, ttyState *[256]string) error {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + id + "/"
	err := os.Mkdir(datadir, 0644|os.ModeSticky)
	if err != nil {
		return logger.MakeError("Failed to create container-specific directory %s. Error: %v", datadir, err)
	}
	logger.Info("Created container-specific directory %s\n", datadir)

	c, err := cli.ContainerInspect(ctx, id)
	if err != nil {
		return logger.MakeError("Error running ContainerInspect on container %s: %v", id, err)
	}

	// Keep track of port mapping
	// We only need to keep track of the mapping of the container's tcp 32262 on the host
	hostPort, exists := c.NetworkSettings.Ports["32262/tcp"]
	if !exists {
		return logger.MakeError("Could not find mapping for port 32262/tcp for container %s", id)
	}
	if len(hostPort) != 1 {
		return logger.MakeError("The hostPort mapping for port 32262/tcp for container %s has length not equal to 1!. Mapping: %+v", id, hostPort)
	}
	err = writeAssignmentToFile(datadir+"hostPort_for_my_32262_tcp", hostPort[0].HostPort)
	if err != nil {
		// Don't need to wrap err here because writeAssignmentToFile already contains the relevant info
		return err
	}

	// Assign an unused tty
	assignedTty := -1
	for tty := range ttyState {
		if ttyState[tty] == "" {
			assignedTty = tty
			ttyState[assignedTty] = id
			break
		}
	}
	if assignedTty == -1 {
		return logger.MakeError("Was not able to assign a free tty to container id %s", id)
	}

	// Write the tty assignment to a file
	err = writeAssignmentToFile(datadir+"tty", logger.Sprintf("%d", assignedTty))
	if err != nil {
		// Don't need to wrap err here because writeAssignmentToFile already contains the relevant info
		return err
	}

	// Indicate that we are ready for the container to read the data back
	err = writeAssignmentToFile(datadir+".ready", ".ready")
	if err != nil {
		// Don't need to wrap err here because writeAssignmentToFile already contains the relevant info
		return err
	}

	return nil
}

func containerDieHandler(ctx context.Context, cli *client.Client, id string, ttyState *[256]string) error {
	// Delete the container-specific data directory we used
	datadir := resourceMappingDirectory + id + "/"
	err := os.RemoveAll(datadir)
	if err != nil {
		return logger.MakeError("Failed to delete container-specific directory %s", datadir)
	}
	logger.Info("Successfully deleted container-specific directory %s\n", datadir)

	for tty := range ttyState {
		if ttyState[tty] == id {
			ttyState[tty] = ""
		}
	}

	return nil
}

func main() {
	// This needs to be the first statement in main(). This deferred function
	// allows us to catch any panics in the main goroutine and therefore execute
	// code on shutdown of the host service. In particular, we want to send a
	// message to Sentry and/or the fractal webserver upon our death.
	defer shutdownHostService()

	// Initialize Sentry. We do this right after the above defer so that we can
	// capture and log the potential error of starting the service as a non-root
	// user.
	//
	// Note that according to the Sentry Go documentation, if we run
	// sentry.CaptureMessage or sentry.CaptureError on separate goroutines, they
	// can overwrite each other's tags. The thread-safe solution is to use
	// goroutine-local hubs, but the way to do that would be to use contexts and
	// add an additional argument to each logging method taking in the current
	// context. This seems like a lot of work, so we just use a set of global
	// tags instead, initializing them in InitializeSentry() and not touching
	// them afterwards. Any container-specific information (which is what I
	// imagine we would use local tags for) we just add in the text of the
	// respective error message sent to Sentry. Alternatively, we might just be
	// able to use sentry.WithScope()
	err := logger.InitializeSentry()
	if err != nil {
		logger.Panicf("Unable to initialize sentry. Error: %s", err)
	}
	// We flush Sentry's queue in shutdownHostService(), so we don't need to defer it here

	// After the above defer and initialization of Sentry, this needs to be the
	// next line of code that runs, since the following operations will require
	// root permissions.
	checkRunningPermissions()

	rand.Seed(time.Now().UnixNano())

	// Note that we defer uninitialization so that in case of panic elsewhere, we
	// still clean up
	initializeFilesystem()
	defer uninitializeFilesystem()

	// Register a signal handler for Ctrl-C so that we still cleanup if Ctrl-C is pressed
	sigChan := make(chan os.Signal, 2)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	go func() {
		// See our note about Sentry and goroutines above.
		defer shutdownHostService()
		<-sigChan
		logger.Info("Got an interrupt or SIGTERM --- calling uninitializeFilesystem() and panicking to initiate host shutdown process...")
		uninitializeFilesystem()
		logger.Panicf("Got a Ctrl+C: already uninitialized filesystem, looking to exit")
	}()

	// Initialize webserver and heartbeat
	err = webserver.InitializeHeartbeat()
	if err != nil {
		logger.Panicf("Unable to initialize webserver. Error: %s", err)
	}

	// Start Docker Daemons and ECS Agent,
	// Notably, this needs to happen after the webserver handshake above. This
	// prevents AWS from assigning any task definitions to our container before
	// the webserver knows about it.
	startDockerDaemon()
	// Only start the ECS Agent if running in production, on an AWS EC2 instance
	if env := os.Getenv("APP_ENV"); env == "production" || env == "prod" {
		logger.Infof("Running in production, starting ECS Agent.")
		startECSAgent()
	}

	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv)
	if err != nil {
		logger.Panicf("Error creating new Docker client: %v", err)
	}

	filters := filters.NewArgs()
	filters.Add("type", events.ContainerEventType)
	eventOptions := types.EventsOptions{
		Filters: filters,
	}

	// reserve the first 10 TTYs for the host system
	const r = "reserved"
	ttyState := [256]string{r, r, r, r, r, r, r, r, r, r}

	// In the following loop, this var determines whether to re-initialize the
	// event stream. This is necessary because the Docker event stream needs to
	// be reopened after any error is sent over the error channel.
	needToReinitializeEventStream := false
	events, errs := cli.Events(context.Background(), eventOptions)
	logger.Info("Initialized event stream...")

eventLoop:
	for {
		if needToReinitializeEventStream {
			events, errs = cli.Events(context.Background(), eventOptions)
			needToReinitializeEventStream = false
			logger.Info("Re-initialized event stream...")
		}

		select {
		case err := <-errs:
			needToReinitializeEventStream = true
			switch {
			case err == nil:
				logger.Info("We got a nil error over the Docker event stream. This might indicate an error inside the docker go library. Ignoring it and proceeding normally...")
				continue
			case err == io.EOF:
				logger.Panicf("Docker event stream has been completely read.")
				break eventLoop
			case client.IsErrConnectionFailed(err):
				// This means "Cannot connect to the Docker daemon..."
				logger.Info("Got error \"%v\". Trying to start Docker daemon ourselves...", err)
				startDockerDaemon()
				continue
			default:
				logger.Panicf("Got an unknown error from the Docker event stream: %v", err)
			}

		case event := <-events:
			if event.Action == "die" || event.Action == "start" {
				logger.Info("Event: %s for %s %s\n", event.Action, event.Type, event.ID)
			}
			if event.Action == "die" {
				err := containerDieHandler(ctx, cli, event.ID, &ttyState)
				if err != nil {
					logger.Errorf("Error processing event %s for %s %s: %v", event.Action, event.Type, event.ID, err)
				}
			}
			if event.Action == "start" {
				err := containerStartHandler(ctx, cli, event.ID, &ttyState)
				if err != nil {
					logger.Errorf("Error processing event %s for %s %s: %v", event.Action, event.Type, event.ID, err)
				}
			}
		}
	}
}
