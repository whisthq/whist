package main

import (
	"bytes"
	"math"
	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via sentry. Instead, use the
	// fractallogger package imported below as `logger`
	"context"
	"encoding/json"
	"io"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"regexp"
	"strconv"
	"strings"
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this impoted package as well.
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	webserver "github.com/fractal/fractal/ecs-host-service/fractalwebserver"

	httpserver "github.com/fractal/fractal/ecs-host-service/httpserver"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/events"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/client"
)

// The location on disk where we store the container resource allocations
const resourceMappingDirectory = "/fractal/containerResourceMappings/"
const cloudStorageDirectory = "/fractal/cloudStorage/"

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

// ---------------------------
// Container state manangement
// ---------------------------

// reserve the first 10 TTYs for the host system
var ttyState [256]string = [256]string{"reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved"}

// keep track of the mapping from hostPort to Docker container ID
var containerIDs map[uint16]string = make(map[uint16]string)

// keys: hostPort, values: slice containing all cloud storage directories that are mounted for that specific container
var cloudStorageDirs map[uint16]map[string]interface{} = make(map[uint16]map[string]interface{})

func unmountCloudStorageDir(hostPort uint16, path string) {
	// Unmount lazily, i.e. will unmount as soon as the directory is not busy
	cmd := exec.Command("fusermount", "-u", "-z", path)
	res, err := cmd.CombinedOutput()
	if err != nil {
		logger.Errorf("Command \"%s\" returned an error code. Output: %s", cmd, res)
	} else {
		logger.Infof("Successfully unmounted cloud storage directory %s", path)
	}

	// Remove it from the state
	_, ok := cloudStorageDirs[hostPort]
	if !ok {
		logger.Infof("No cloud storage dirs found for hostPort %v", hostPort)
		return
	}
	delete(cloudStorageDirs[hostPort], path)
}

// Mounts the cloud storage directory and waits around to clean it up once it's unmounted.
func mountCloudStorageDir(req *httpserver.MountCloudStorageRequest) error {
	sanitized_provider := func() string {
		p := strings.ReplaceAll(req.Provider, " ", "_")
		p = regexp.MustCompile("[^a-zA-Z0-9_]+").ReplaceAllString(p, "")
		return p
	}()

	// Don't forget the trailing slash
	hostPort := logger.Sprintf("%v", req.HostPort)
	path := cloudStorageDirectory + hostPort + "/" + sanitized_provider + "/"
	configName := hostPort + sanitized_provider

	token, err := func() (string, error) {
		buf, err := json.Marshal(
			struct {
				AccessToken  string `json:"access_token"`
				TokenType    string `json:"token_type"`
				RefreshToken string `json:"refresh_token"`
				Expiry       string `json:"expiry"`
			}{
				req.AccessToken,
				req.TokenType,
				req.RefreshToken,
				req.Expiry,
			},
		)
		if err != nil {
			return "", logger.MakeError("Error creating token for rclone: %s", err)
		}

		return logger.Sprintf("'%s'", buf), nil
	}()
	if err != nil {
		return err
	}

	// Make directory to mount in
	err = os.MkdirAll(path, os.FileMode(0777))
	if err != nil {
		return logger.MakeError("Could not mkdir path %s. Error: %s", path, err)
	} else {
		logger.Infof("Created directory %s", path)
	}

	// We mount in foreground mode, and wait for the result to clean up the
	// directory created for this purpose. That way we know that we aren't
	// accidentally removing files from the user's cloud storage drive.
	// cmd := exec.Command(
	strcmd := strings.Join(
		[]string{
			"/usr/bin/rclone", "config", "create", configName, "drive",
			"config_is_local", "false",
			"config_refresh_token", "false",
			"token", token,
			"scope", "drive",
		}, " ")
	// )
	scriptpath := resourceMappingDirectory + "config-create-" + configName + ".sh"
	f, _ := os.Create(scriptpath)
	_, _ = f.WriteString(logger.Sprintf("#!/bin/sh\n\n"))
	_, _ = f.WriteString(strcmd)
	os.Chmod(scriptpath, os.FileMode(0777))
	f.Close()
	defer os.RemoveAll(scriptpath)
	cmd := exec.Command("runuser", "ubuntu", scriptpath)

	logger.Info("Rclone config create command: %v", cmd)

	output, err := cmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Could not run \"rclone config create\" command: %s. Output: %s", err, output)
	} else {
		logger.Info("Ran \"rclone config create\" command with output: %s", output)
	}

	// Mount in separate goroutine so we don't block the main goroutine.
	// Synchronize using errorchan.
	errorchan := make(chan error)
	go func() {
		// cmd = exec.Command("runuser", "ubuntu", "-c", "'/usr/bin/rclone mount "+configName+":/ "+path+"'")
		cmd = exec.Command("rclone", "mount", configName+":/", path)
		cmd.Env = os.Environ()
		logger.Info("Rclone mount command: [  %v  ]", cmd)
		stderr, _ := cmd.StderrPipe()

		if _, ok := cloudStorageDirs[uint16(req.HostPort)]; !ok {
			cloudStorageDirs[uint16(req.HostPort)] = make(map[string]interface{}, 0)
		}
		cloudStorageDirs[uint16(req.HostPort)][path] = nil

		err = cmd.Start()
		if err != nil {
			errorchan <- logger.MakeError("Could not start \"rclone mount %s\" command: %s", configName+":/", err)
			close(errorchan)
			return
		}

		// We close errorchan after 1000ms so the enclosing function can return an
		// error if the `rclone mount` command failed immediately, or return nil if
		// it didn't.
		timeout := time.Second * 5
		timer := time.AfterFunc(timeout, func() { close(errorchan) })
		logger.Infof("Attempting to mount storage directory %s", path)

		errbuf := new(bytes.Buffer)
		errbuf.ReadFrom(stderr)

		err = cmd.Wait()
		if err != nil {
			errorchanStillOpen := timer.Stop()
			if errorchanStillOpen {
				errorchan <- logger.MakeError("Mounting of cloud storage directory %s returned an error: %s. Output: %s", path, err, errbuf)
				close(errorchan)
			} else {
				logger.Errorf("Mounting of cloud storage directory %s failed after more than timeout %v and was therefore not reported as a failure to the webserver. Error: %s. Output: %s", path, timeout, err, errbuf)
			}
		}

		// Remove the now-unnecessary directory we created
		err = os.Remove(path)
		if err != nil {
			logger.Errorf("Error removing cloud storage directory %s: %s", path, err)
		}
	}()

	err = <-errorchan
	return err
}

func handleDPIRequest(req *httpserver.SetContainerDPIRequest) error {
	// Compute container-specific directory to write DPI data to
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		return logger.MakeError("Invalid HostPort for DPI request: %v", req.HostPort)
	}
	hostPort := (uint16)(req.HostPort)
	id, exists := containerIDs[hostPort]
	if !exists {
		return logger.MakeError("Could not find currently-starting container with hostPort %v", hostPort)
	}
	datadir := resourceMappingDirectory + id + "/"

	// Actually write DPI information to file
	strdpi := logger.Sprintf("%v", req.DPI)
	filename := datadir + "DPI"
	err := writeAssignmentToFile(filename, strdpi)
	if err != nil {
		return logger.MakeError("Could not write value %v to DPI file %v. Error: %s", strdpi, filename, err)
	}

	// Indicate that we are ready for the container to read the data back
	// (see comment at the end of containerStartHandler)
	err = writeAssignmentToFile(datadir+".ready", ".ready")
	if err != nil {
		// Don't need to wrap err here because writeAssignmentToFile already contains the relevant info
		return err
	}

	return nil
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

func containerStartHandler(ctx context.Context, cli *client.Client, id string) error {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + id + "/"
	err := os.Mkdir(datadir, os.FileMode(0777))
	if err != nil {
		return logger.MakeError("Failed to create container-specific directory %s. Error: %v", datadir, err)
	}
	logger.Info("Created container-specific directory %s\n", datadir)

	c, err := cli.ContainerInspect(ctx, id)
	if err != nil {
		return logger.MakeError("Error running ContainerInspect on container %s: %v", id, err)
	}

	// We ignore the ecs-agent container, since we don't need to do anything to
	// it, and we want to avoid triggering an error that '32262/tcp' is unmapped
	// (which gets sent to Sentry).
	if c.Name == "ecs-agent" {
		logger.Info("Detected ecs-agent starting. Doing nothing.")
		return nil
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

	// If we didn't run into any errors so far, we add this container to the
	// `containerIDs` map.
	hostPortInt64, err := strconv.ParseUint(hostPort[0].HostPort, 10, 16)
	if err != nil {
		err := logger.MakeError("containerStartHandler: The hostPort %s for container %s did not parse into a uint16!", hostPort, id)
		return err
	}
	hostPortUint16 := uint16(hostPortInt64)
	containerIDs[hostPortUint16] = id

	// We do not mark the container as "ready" but instead let handleDPIRequest
	// do that, since we don't want to mark a container as ready until the DPI is
	// set. We are confident that the DPI request will arrive after the Docker
	// start handler is called, since the DPI request is not triggered until the
	// webserver receives the hostPort from AWS, which does not happen until the
	// container is in a "RUNNING" state.

	return nil
}

func containerDieHandler(ctx context.Context, cli *client.Client, id string) {
	// Delete the container-specific data directory we used
	datadir := resourceMappingDirectory + id + "/"
	err := os.RemoveAll(datadir)
	if err != nil {
		logger.Errorf("Failed to delete container-specific directory %s", datadir)
		// Do not return here, since we still want to de-allocate the TTY if it exists
	}
	logger.Info("Successfully deleted (possibly non-existent) container-specific directory %s\n", datadir)

	// Free tty internal state
	for tty := range ttyState {
		if ttyState[tty] == id {
			ttyState[tty] = ""
		}
	}

	// Make sure the container is removed from the `containerIDs` map
	// Note that we cannot parse the hostPort the same way we do in
	// containerStartHandler, since now the hostPort does not appear in the
	// container's data! Therefore, we have to find it by id.
	var hostPort uint16
	foundHostPort := false
	for k, v := range containerIDs {
		if v == id {
			foundHostPort = true
			hostPort = k
			break
		}
	}
	if !foundHostPort {
		logger.Errorf("Could not find a hostPort mapping for container %s", id)
		return
	}
	delete(containerIDs, hostPort)
	logger.Infof("Deleted mapping from hostPort %v to container ID %v", hostPort, id)

	// Unmount cloud storage directories
	storageDirs, exists := cloudStorageDirs[hostPort]
	if exists {
		for k := range storageDirs {
			unmountCloudStorageDir(hostPort, k)
		}
	}
}

// ---------------------------
// Service shutdown and initialization
// ---------------------------

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

	// Create the resource mapping directory
	err := os.MkdirAll(resourceMappingDirectory, os.FileMode(0777))
	if err != nil {
		logger.Panicf("Failed to create directory %s: error: %s\n", resourceMappingDirectory, err)
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
	err = os.MkdirAll(httpserver.FractalPrivatePath, os.FileMode(0777))
	if err != nil {
		logger.Panicf("Failed to create directory %s: error: %s\n", httpserver.FractalPrivatePath, err)
	}
}

func uninitializeFilesystem() {
	err := os.RemoveAll(resourceMappingDirectory)
	if err != nil {
		logger.Panicf("Failed to delete directory %s: error: %v\n", resourceMappingDirectory, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", resourceMappingDirectory)
	}

	err = os.RemoveAll(httpserver.FractalPrivatePath)
	if err != nil {
		logger.Panicf("Failed to delete directory %s: error: %v\n", httpserver.FractalPrivatePath, err)
	} else {
		logger.Infof("Successfully deleted directory %s\n", httpserver.FractalPrivatePath)
	}

	// Unmount all cloud-storage folders
	for port, dirs := range cloudStorageDirs {
		for k := range dirs {
			unmountCloudStorageDir(port, k)
		}
	}
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
	// able to use sentry.WithScope(), but that is future work.
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

	// Log the Git commit of the running executable
	logger.Info("Host Service Version: %s", logger.GetGitCommit())

	// Initialize webserver heartbeat
	err = webserver.InitializeHeartbeat()
	if err != nil {
		logger.Panicf("Unable to initialize webserver. Error: %s", err)
	}

	// Start the HTTP server and listen for events
	serverEvents, err := httpserver.StartHTTPSServer()
	if err != nil {
		logger.Panic(err)
	}

	// Start Docker Daemons and ECS Agent,
	// Notably, this needs to happen after the webserver handshake above. This
	// prevents AWS from assigning any task definitions to our container before
	// the webserver knows about it.
	startDockerDaemon()
	// Only start the ECS Agent if running in production, on an AWS EC2 instance
	if logger.GetAppEnvironment() == logger.EnvProd {
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

	// In the following loop, this var determines whether to re-initialize the
	// event stream. This is necessary because the Docker event stream needs to
	// be reopened after any error is sent over the error channel.
	needToReinitializeEventStream := false
	dockerevents, dockererrs := cli.Events(context.Background(), eventOptions)
	logger.Info("Initialized event stream...")

eventLoop:
	for {
		if needToReinitializeEventStream {
			dockerevents, dockererrs = cli.Events(context.Background(), eventOptions)
			needToReinitializeEventStream = false
			logger.Info("Re-initialized event stream...")
		}

		select {
		case err := <-dockererrs:
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

		case dockerevent := <-dockerevents:
			if dockerevent.Action == "die" || dockerevent.Action == "start" {
				logger.Info("dockerevent: %s for %s %s\n", dockerevent.Action, dockerevent.Type, dockerevent.ID)
			}
			if dockerevent.Action == "start" {
				// We want the container start handler to die immediately upon failure,
				// so it returns an error as soon as it encounters one.
				err := containerStartHandler(ctx, cli, dockerevent.ID)
				if err != nil {
					logger.Errorf("Error processing dockerevent %s for %s %s: %v", dockerevent.Action, dockerevent.Type, dockerevent.ID, err)
				}
			} else if dockerevent.Action == "die" {
				// Since we want all steps in the die handler to be attempted,
				// regardless of earlier errors, we let the containerDieHandler report
				// its own errors, and return nothing to us.
				containerDieHandler(ctx, cli, dockerevent.ID)
			}

		case serverevent := <-serverEvents:
			switch serverevent.(type) {
			case *httpserver.SetContainerDPIRequest:
				err := handleDPIRequest(serverevent.(*httpserver.SetContainerDPIRequest))
				if err != nil {
					logger.Error(err)
				}
				serverevent.ReturnResult("", err)

			case *httpserver.MountCloudStorageRequest:
				err := mountCloudStorageDir(serverevent.(*httpserver.MountCloudStorageRequest))
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
