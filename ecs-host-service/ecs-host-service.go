package main

import (
	"bytes"
	"math"

	// NOTE: The "fmt" or "log" packages should never be imported!!! This is so
	// that we never forget to send a message via Sentry. Instead, use the
	// fractallogger package imported below as `logger`
	"context"
	"encoding/json"
	"io"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"syscall"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this impoted package as well.
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	ecsagent "github.com/fractal/fractal/ecs-host-service/ecsagent"
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
const fractalTempDir = fractalDir + "temp/"
const userConfigs = "userConfigs/"

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
// Container state manangement/mappings
// ------------------------------------

// keys: hostPort, values: slice containing all cloud storage directories that are
// mounted for that specific container
var cloudStorageDirs map[uint16]map[string]interface{} = make(map[uint16]map[string]interface{})

// Unmounts a cloud storage directory mounted on hostPort
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
	var rcloneType string
	var dirName string

	switch req.Provider {
	case "google_drive":
		rcloneType = "drive"
		dirName = "Google Drive"
	case "dropbox":
		rcloneType = "dropbox"
		dirName = "Dropbox"
	default:
		return logger.MakeError("Unrecognized cloud storage provider: req.Provider=%s", req.Provider)
	}

	// Compute the directory to mount the cloud storage folder in
	dockerID, ok := containerIDs[uint16(req.HostPort)]
	if !ok {
		return logger.MakeError("mountCloudStoragedir: couldn't find DockerID for hostPort %v", req.HostPort)
	}
	fractalID, ok := fractalIDs[dockerID]
	if !ok {
		return logger.MakeError("mountCloudStoragedir: couldn't find FractalID for hostPort %v", req.HostPort)
	}

	// Don't forget the trailing slash
	path := logger.Sprintf("%s%s/%s/", cloudStorageDir, fractalID, dirName)
	configName := fractalID + rcloneType

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
		return logger.MakeError("Error creating token for rclone: %s", err)
	}
	token := logger.Sprintf("'%s'", buf)

	// Make directory to mount in
	err = os.MkdirAll(path, 0777)
	if err != nil {
		return logger.MakeError("Could not mkdir path %s. Error: %s", path, err)
	}
	logger.Infof("Created directory %s", path)
	makeFractalDirectoriesFreeForAll()

	// We mount in foreground mode, and wait for the result to clean up the
	// directory created for this purpose. That way we know that we aren't
	// accidentally removing files from the user's cloud storage drive.
	// cmd := exec.Command(
	strcmd := strings.Join(
		[]string{
			"/usr/bin/rclone", "config", "create", configName, rcloneType,
			"config_is_local", "false",
			"config_refresh_token", "false",
			"token", token,
			"client_id", req.ClientID,
			"client_secret", req.ClientSecret,
		}, " ")
	// )
	scriptpath := fractalTempDir + "config-create-" + configName + ".sh"
	f, _ := os.Create(scriptpath)
	_, _ = f.WriteString(logger.Sprintf("#!/bin/sh\n\n"))
	_, _ = f.WriteString(strcmd)
	os.Chmod(scriptpath, 0700)
	f.Close()
	defer os.RemoveAll(scriptpath)
	cmd := exec.Command(scriptpath)

	logger.Info("Rclone config create command: %v", cmd)

	output, err := cmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Could not run \"rclone config create\" command: %s. Output: %s", err, output)
	}
	logger.Info("Ran \"rclone config create\" command with output: %s", output)

	// Mount in separate goroutine so we don't block the main goroutine.
	// Synchronize using errorchan.
	errorchan := make(chan error)
	go func() {
		cmd = exec.Command("/usr/bin/rclone", "mount", configName+":/", path, "--allow-other", "--vfs-cache-mode", "writes")
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

		// We close errorchan after `timeout` so the enclosing function can return an
		// error if the `rclone mount` command failed immediately, or return nil if
		// it didn't.
		timeout := time.Second * 15
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

// When container disconnects, re-sync the user config back to S3
// and delete the folder.
// Takes fractalID (fractal id for the container) as an argument.
func saveUserConfig(fractalID string) {
	appName, ok := containerAppNames[fractalID]
	if !ok {
		logger.Infof("No app name found for FractalID %v", fractalID)
		return
	}

	userID, ok := containerUserIDs[fractalID]
	if !ok {
		logger.Infof("No user ID found for FractalID %v", fractalID)
		return
	}

	// Save app config back to s3 - first tar, then upload
	configPath := fractalDir + fractalID + "/" + userConfigs
	s3ConfigPath := "s3://fractal-user-app-configs/" + userID + "/" + string(appName) + "/"

	// Only tar and save the config back to S3 if the user ID is set
	if userID != "" {
		tarPath := configPath + "fractal-app-config.tar.gz"

		tarConfigCmd := exec.Command("/usr/bin/tar", "-C", configPath, "-czf", tarPath, "--exclude=fractal-app-config.tar.gz", ".")
		tarConfigOutput, err := tarConfigCmd.CombinedOutput()
		// tar is only fatal when exit status is 2 -
		//    exit status 1 just means that some files have changed while tarring,
		//    which is an ignorable error
		if err != nil && !strings.Contains(string(tarConfigOutput), "file changed") {
			logger.Errorf("Could not tar config directory: %s. Output: %s", err, tarConfigOutput)
		} else {
			logger.Infof("Tar config directory output: %s", tarConfigOutput)
		}

		saveConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", tarPath, s3ConfigPath)
		saveConfigOutput, err := saveConfigCmd.CombinedOutput()
		if err != nil {
			logger.Errorf("Could not run \"aws s3 cp\" save config command: %s. Output: %s", err, saveConfigOutput)
		} else {
			logger.Infof("Ran \"aws s3 cp\" save config command with output: %s", saveConfigOutput)
		}
	}

	// remove app name mapping for container on fractalID
	delete(containerAppNames, fractalID)

	// clear contents of config directory
	os.RemoveAll(configPath)
}

// Populate the config folder under the container's FractalID for the
// container's attached user and running application.
// Takes the request to the `set_container_start_values` endpoint as
// and argument and returns nil if no errors, and error object if error.
func getUserConfig(req *httpserver.SetContainerStartValuesRequest) error {
	// Get needed vars and create path for config
	userID := req.UserID
	containerID := containerIDs[(uint16)(req.HostPort)]
	fractalID := fractalIDs[containerID]
	appName := containerAppNames[fractalID]
	configPath := fractalDir + fractalID + "/" + userConfigs

	// Make directory to move configs to
	err := os.MkdirAll(configPath, 0777)
	if err != nil {
		return logger.MakeError("Could not mkdir path %s. Error: %s", configPath, err)
	}
	logger.Infof("Created directory %s", configPath)

	makeFractalDirectoriesFreeForAll()

	// Store app name and user ID in maps
	containerUserIDs[fractalID] = string(userID)

	// If userID is not set, we don't want to try to retrieve configs from S3
	if userID != "" {
		s3ConfigPath := "s3://fractal-user-app-configs/" + userID + "/" + appName + "/fractal-app-config.tar.gz"
		// Retrieve app config from S3
		getConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", s3ConfigPath, configPath)
		getConfigOutput, err := getConfigCmd.CombinedOutput()
		// If aws s3 cp errors out due to the file not existing, don't log an error because
		//    this means that it's the user's first run and they don't have any settings
		//    stored for this application yet.
		if err != nil {
			if strings.Contains(string(getConfigOutput), "does not exist") {
				logger.Infof("Ran \"aws s3 cp\" and config does not exist")
				return nil
			}
			return logger.MakeError("Could not run \"aws s3 cp\" get config command: %s. Output: %s", err, getConfigOutput)
		}
		logger.Infof("Ran \"aws s3 cp\" get config command with output: %s", getConfigOutput)

		// Extract the config archive to the user config directory
		tarPath := configPath + "fractal-app-config.tar.gz"
		untarConfigCmd := exec.Command("/usr/bin/tar", "-xzf", tarPath, "-C", configPath)
		untarConfigOutput, err := untarConfigCmd.CombinedOutput()
		if err != nil {
			logger.Errorf("Could not untar config archive: %s. Output: %s", err, untarConfigOutput)
		} else {
			logger.Infof("Untar config directory output: %s", untarConfigOutput)
		}
	}
	return nil
}

// Creates a file containing the DPI assigned to a specific container, and make
// it accessible to that container. Also take the received User ID and retrieve
// the user's app configs if the User ID is set.
// Takes the request to the `set_container_start_values` endpoint as
// and argument and returns nil if no errors, and error object if error.
func handleStartValuesRequest(req *httpserver.SetContainerStartValuesRequest) error {
	// Compute container-specific directory to write start value data to
	if req.HostPort > math.MaxUint16 || req.HostPort < 0 {
		return logger.MakeError("Invalid HostPort for start values request: %v", req.HostPort)
	}
	hostPort := uint16(req.HostPort)
	id, exists := containerIDs[hostPort]
	if !exists {
		return logger.MakeError("Could not find currently-starting container with hostPort %v", hostPort)
	}

	// Get the fractalID and use it to compute the right resource mapping directory
	fractalID, ok := fractalIDs[id]
	if !ok {
		// This is actually an error, since we received a DPI request.
		return logger.MakeError("handleDPIRequest(): couldn't find FractalID mapping for container with DockerID %s", id)
	}
	datadir := fractalDir + fractalID + "/" + containerResourceMappings

	// Actually write DPI information to file
	strdpi := logger.Sprintf("%v", req.DPI)
	filename := datadir + "DPI"
	err := writeAssignmentToFile(filename, strdpi)
	if err != nil {
		return logger.MakeError("Could not write value %v to DPI file %v. Error: %s", strdpi, filename, err)
	}

	// Actually write Container ARN information to file
	idFilename := datadir + "ContainerARN"
	err = writeAssignmentToFile(idFilename, req.ContainerARN)
	if err != nil {
		return logger.MakeError("Could not write value %v to ID file %v. Error: %s", req.ContainerARN, filename, err)
	}

	// Populate the user config folder for the container's app
	err = getUserConfig(req)
	if err != nil {
		logger.Error(err)
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

// Handle tasks to be completed when a container dies
func containerDieHandler(ctx context.Context, cli *dockerclient.Client, id string) {
	// Get the fractalID and use it to compute the right data directory. Also,
	// exit if we are not dealing with a Fractal container.
	fractalID, ok := fractalIDs[id]
	if !ok {
		logger.Infof("containerDieHandler(): couldn't find FractalID mapping for container with DockerID %s", id)
		return
	}

	// Delete the container-specific data directory we used
	datadir := fractalDir + fractalID + "/" + containerResourceMappings
	err := os.RemoveAll(datadir)
	if err != nil {
		logger.Errorf("containerDieHandler(): Failed to delete container-specific directory %s", datadir)
		// Do not return here, since we still want to de-allocate the TTY if it exists
	}
	logger.Info("containerDieHandler(): Successfully deleted (possibly non-existent) container-specific directory %s\n", datadir)

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
		logger.Infof("containerDieHandler(): Could not find a hostPort mapping for container %s", id)
		return
	}

	// Delete user config and resave to S3
	saveUserConfig(fractalID)

	resourcetrackers.FreePortBindings(fractalID)
	delete(containerIDs, hostPort)
	logger.Infof("containerDieHandler(): Deleted mapping from hostPort %v to container ID %v", hostPort, id)

	// Unmount cloud storage directories
	storageDirs, exists := cloudStorageDirs[hostPort]
	if exists {
		for k := range storageDirs {
			unmountCloudStorageDir(hostPort, k)
		}
	}

	// Delete associated devices and remove them from the map
	uinputDevices, ok := devices[fractalID]
	if !ok {
		logger.Infof("containerDieHandler(): Couldn't find uinput devices mapping for container with fractalID %s", fractalID)
		return
	}
	uinputDevices.absmouse.Close()
	uinputDevices.relmouse.Close()
	uinputDevices.keyboard.Close()
	delete(devices, fractalID)
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

	// Unmount all cloud-storage folders
	for port, dirs := range cloudStorageDirs {
		for k := range dirs {
			unmountCloudStorageDir(port, k)
		}
	}
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
		logger.Panicf("Unable to initialize webserver. Error: %s", err)
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
