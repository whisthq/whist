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
	"net"
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"
	"unsafe"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via Sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this impoted package as well.
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"

	ecsagent "github.com/fractal/fractal/ecs-host-service/ecsagent"
	fractaltypes "github.com/fractal/fractal/ecs-host-service/fractaltypes"
	webserver "github.com/fractal/fractal/ecs-host-service/fractalwebserver"
	httpserver "github.com/fractal/fractal/ecs-host-service/httpserver"
	resourcetrackers "github.com/fractal/fractal/ecs-host-service/resourcetrackers"

	uinput "github.com/fractal/uinput-go"

	dockertypes "github.com/docker/docker/api/types"
	dockerevents "github.com/docker/docker/api/types/events"
	dockerfilters "github.com/docker/docker/api/types/filters"
	dockerclient "github.com/docker/docker/client"
)

// The locations on disk where we store our data, including cloud storage
// mounts and container resource allocations. Note that we keep the cloud
// storage in its own directory, not in the fractalDir, so that we can safely
// delete the entire `fractal` directory on exit.
const fractalDir = "/fractal/"
const cloudStorageDir = "/fractalCloudStorage/"
const fractalTempDir = fractalDir + "temp/"
const containerResourceMappings = "containerResourceMappings/"
const userConfigs = "userConfigs/"

// The filenames (NOT paths) for the encrypted and decrypted user config archives
const encArchiveFilename = "fractal-app-config.tar.gz.enc"
const decArchiveFilename = "fractal-app-config.tar.gz"

// TODO: get rid of this security nemesis
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

// Check that the program has been started with the correct permissions --- for
// now, we just want to run as root, but this service could be assigned its own
// user in the future
func checkRunningPermissions() {
	if os.Geteuid() != 0 {
		logger.Panicf("This service needs to run as root!")
	}
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

// reserve the first 10 TTYs for the host system
var ttyState [256]string = [256]string{"reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved"}

// keep track of the mapping from hostPort to Docker container ID
var containerIDs map[uint16]string = make(map[uint16]string)

// keep track of the mapping from Docker container ID to FractalID (a
// unique identifier for each container created by our modified ecs-agent
var fractalIDs map[string]string = make(map[string]string)

// keep track of the mapping from FractalID to app (e.g. browsers/chrome)
var containerAppNames map[string]string = make(map[string]string)

// keep track of the mapping from FractalID to UserID
var containerUserIDs map[string]string = make(map[string]string)

// keys: hostPort, values: slice containing all cloud storage directories that are
// mounted for that specific container
var cloudStorageDirs map[uint16]map[string]interface{} = make(map[uint16]map[string]interface{})

type uinputDevices struct {
	absmouse uinput.TouchPad
	relmouse uinput.Mouse
	keyboard uinput.Keyboard
}

// keep track of mapping from FractalID to uinput devices
var devices map[string]uinputDevices = make(map[string]uinputDevices)

// we keep track of mapping from FractalIDs to host ports in `resourcetrackers.PortBindings`
// TODO(djsavvy): move other tracked resources to the `resourcetrackers` package.

// Updates the fractalIDs mapping with a request from the ecs-agent
func addFractalIDMappings(req *httpserver.RegisterDockerContainerIDRequest) error {
	if req.DockerID == "" || req.FractalID == "" || req.AppName == "" {
		return logger.MakeError("Got a RegisterDockerContainerIDRequest with an empty field!. req.DockerID: \"%s\", req.FractalID: \"%s\", req.AppName: \"%s\"",
			req.DockerID, req.FractalID, req.AppName)
	}

	fractalIDs[req.DockerID] = req.FractalID
	containerAppNames[req.FractalID] = req.AppName
	logger.Infof("Added mapping from DockerID %s to FractalID %s", req.DockerID, req.FractalID)
	return nil
}

func linuxUIGetSysName(len uintptr) uintptr {
	// from ioctl.h and uinput.h
	const (
		iocDirshift  = 30
		iocSizeshift = 16
		iocTypeshift = 8
		iocNrshift   = 0
		iocRead      = 2
		uiIoctlBase  = 85 // 'U'
	)

	linuxIoc := func(dir uintptr, requestType uintptr, nr uintptr, size uintptr) uintptr {
		return (dir << iocDirshift) + (requestType << iocTypeshift) + (nr << iocNrshift) + (size << iocSizeshift)
	}

	return linuxIoc(iocRead, uiIoctlBase, 44, len)
}

// getDeviceFilePath returns the file path (e.g. /dev/input/event3) given a file descriptor corresponding to a virtual device created with uinput
func getDeviceFilePath(fd *os.File) (string, error) {
	const maxlen uintptr = 32
	bsysname := make([]byte, maxlen)
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(fd.Fd()), linuxUIGetSysName(maxlen), uintptr(unsafe.Pointer(&bsysname[0])))
	if errno != 0 {
		return "", logger.MakeError("ioctl to get sysname failed with errno %d", errno)
	}
	sysname := string(bytes.Trim(bsysname, "\x00"))
	syspath := logger.Sprintf("/sys/devices/virtual/input/%s", sysname)
	sysdir, err := os.Open(syspath)
	if err != nil {
		return "", logger.MakeError("could not open directory %s: %s", syspath, err)
	}
	names, err := sysdir.Readdirnames(0)
	if err != nil {
		return "", logger.MakeError("Readdirnames for directory %s failed: %s", syspath, err)
	}
	for _, name := range names {
		if strings.HasPrefix(name, "event") {
			return "/dev/input/" + name, nil
		}
	}
	return "", logger.MakeError("did not find file in %s with prefix 'event'", syspath)
}

// Create uinput devices and pass them back to the ecs-agent
func createUinputDevices(r *httpserver.CreateUinputDevicesRequest) ([]fractaltypes.UinputDeviceMapping, error) {
	FractalID := r.FractalID
	logger.Infof("Processing CreateUinputDevicesRequest for FractalID: %s", FractalID)

	absmouse, err := uinput.CreateTouchPad("/dev/uinput", []byte("Fractal Virtual Absolute Input"), 0, 0xFFF, 0, 0xFFF)
	if err != nil {
		return nil, logger.MakeError("Could not create virtual absolute input: %s", err)
	}
	absmousePath, err := getDeviceFilePath(absmouse.DeviceFile())
	if err != nil {
		return nil, logger.MakeError("Failed to get device path for virtual absolute input: %s", err)
	}

	relmouse, err := uinput.CreateMouse("/dev/uinput", []byte("Fractal Virtual Relative Input"))
	if err != nil {
		logger.Errorf("Could not create virtual relative input: %s", err)
	}
	relmousePath, err := getDeviceFilePath(relmouse.DeviceFile())
	if err != nil {
		return nil, logger.MakeError("Failed to get device path for virtual relative input: %s", err)
	}

	keyboard, err := uinput.CreateKeyboard("/dev/uinput", []byte("Fractal Virtual Keyboard"))
	if err != nil {
		logger.Errorf("Could not create virtual keyboard: %s", err)
	}
	keyboardPath, err := getDeviceFilePath(keyboard.DeviceFile())
	if err != nil {
		return nil, logger.MakeError("Failed to get device path for virtual keyboard: %s", err)
	}

	devices[FractalID] = uinputDevices{absmouse: absmouse, relmouse: relmouse, keyboard: keyboard}

	// set up goroutine to create unix socket and pass file descriptors to protocol
	go func() {
		// TODO: handle errors better
		// TODO: exit goroutine if container dies
		dirname := fractalTempDir + FractalID + "/sockets/"
		filename := dirname + "uinput.sock"
		os.MkdirAll(dirname, 0777)
		os.RemoveAll(filename)
		server, err := net.Listen("unix", filename)
		if err != nil {
			logger.Errorf("Could not create unix socket at %s: %s", filename, err)
			return
		}
		defer server.Close()

		logger.Infof("Successfully created unix socket at: %s", filename)

		// server.Accept() blocks until the protocol connects
		client, err := server.Accept()
		if err != nil {
			logger.Errorf("Could not connect to client over unix socket: %s", err)
			return
		}
		defer client.Close()

		connf, err := client.(*net.UnixConn).File()
		if err != nil {
			logger.Errorf("Could not get file corresponding to client connection: %s", err)
			return
		}
		defer connf.Close()

		connfd := int(connf.Fd())
		fds := [3]int{int(absmouse.DeviceFile().Fd()), int(relmouse.DeviceFile().Fd()), int(keyboard.DeviceFile().Fd())}
		rights := syscall.UnixRights(fds[:]...)
		syscall.Sendmsg(connfd, nil, rights, nil, 0)
		logger.Infof("Sent uinput file descriptors to %s", FractalID)
	}()

	return []fractaltypes.UinputDeviceMapping{
		{
			PathOnHost:        absmousePath,
			PathInContainer:   absmousePath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
		{
			PathOnHost:        relmousePath,
			PathInContainer:   relmousePath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
		{
			PathOnHost:        keyboardPath,
			PathInContainer:   keyboardPath,
			CgroupPermissions: "rwm", // read, write, mknod (the default)
		},
	}, nil
}

// Allocate the necessary host ports and pass them back to the ecs-agent
func allocateHostPorts(r *httpserver.RequestPortBindingsRequest) ([]fractaltypes.PortBinding, error) {
	return resourcetrackers.AllocatePortBindings(r.FractalID, r.Bindings)
}

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
		encTarPath := configPath + encArchiveFilename
		decTarPath := configPath + decArchiveFilename

		tarConfigCmd := exec.Command(
			"/usr/bin/tar", "-C", configPath, "-czf", decTarPath,
			"--exclude=" + encTarPath, "--exclude=" + decTarPath,
			"."
		)
		tarConfigOutput, err := tarConfigCmd.CombinedOutput()
		// tar is only fatal when exit status is 2 -
		//    exit status 1 just means that some files have changed while tarring,
		//    which is an ignorable error
		if err != nil && !strings.Contains(string(tarConfigOutput), "file changed") {
			logger.Errorf("Could not tar config directory: %s. Output: %s", err, tarConfigOutput)
		} else {
			logger.Infof("Tar config directory output: %s", tarConfigOutput)
		}

		//openssl aes-256-cbc -e -in userConfigs/fractal-app-config-test.tar.lz4 -out testaes.enc -pass file:aes-test-password.txt -pbkdf2
		// At this point, config archive must exist: decrypt app config
		encryptConfigCmd := exec.Command(
			"/usr/bin/openssl", "aes-256-cbc", "-e",
			"-in", decTarPath,
			"-out", encTarPath,
			"-pass", "pass:" + userEncryptionKey, "-pbkdf2"
		)
		encryptConfigOutput, err := encryptConfigCmd.CombinedOutput()
		if err != nil {
			return logger.MakeError("Could not encrypt config: %s. Output: %s", err, encryptConfigOutput)
		}

		saveConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", encTarPath, s3ConfigPath)
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
	userEncryptionKey := req.UserEncryptionKey
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
		s3ConfigPath := "s3://fractal-user-app-configs/" + userID + "/" + appName + "/" + encArchiveFilename
		// Retrieve app config from S3
		getConfigCmd := exec.Command("/usr/bin/aws", "s3", "cp", s3ConfigPath, configPath)
		getConfigOutput, err := getConfigCmd.CombinedOutput()
		// If aws s3 cp errors out due to the file not existing, don't log an error because
		//    this means that it's the user's first run and they don't have any settings
		//    stored for this application yet.
		if err != nil {
			if !strings.Contains(string(getConfigOutput), "does not exist") {
				logger.Infof("No config retrieved from \"aws s3 cp\", but no fatal error: %s", getConfigOutput)
				return nil
			} else {
				return logger.MakeError("Could not run \"aws s3 cp\" get config command: %s. Output: %s", err, getConfigOutput)
			}
		}
		logger.Infof("Ran \"aws s3 cp\" get config command with output: %s", getConfigOutput)

		encTarPath := configPath + encArchiveFilename
		decTarPath := configPath + decArchiveFilename

		// At this point, config archive must exist: decrypt app config
		decryptConfigCmd := exec.Command(
			"/usr/bin/openssl", "aes-256-cbc", "-d",
			"-in", encTarPath,
			"-out", decTarPath,
			"-pass", "pass:" + userEncryptionKey, "-pbkdf2"
		)
		decryptConfigOutput, err := decryptConfigCmd.CombinedOutput()
		if err != nil {
			return logger.MakeError("Could not decrypt config: %s. Output: %s", err, decryptConfigOutput)
		}

		// Delete original encrypted config
		err = os.Remove(encTarPath)
		if err != nil {
			logger.Errorf("getUserConfig(): Failed to delete user config encrypted archive %s", encTarPath)
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

// Helper function to write data to a file
func writeAssignmentToFile(filename, data string) (err error) {
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0777)
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

// Starts a container on the host by assigning to it the relevant parameters and
// an unused TTY
func containerStartHandler(ctx context.Context, cli *dockerclient.Client, id string) error {
	c, err := cli.ContainerInspect(ctx, id)
	if err != nil {
		return logger.MakeError("Error running ContainerInspect on container %s: %v", id, err)
	}

	// Create a container-specific directory to store mappings. Exit if it's a
	// non-fractal container.
	//   Get the fractalID and use it to compute the right directory
	fractalID, ok := fractalIDs[id]
	if !ok {
		return logger.MakeError("containerStartHandler(): couldn't find FractalID mapping for container with name %s", c.Name)
	}
	datadir := fractalDir + fractalID + "/" + containerResourceMappings

	err = os.MkdirAll(datadir, 0777)
	if err != nil {
		return logger.MakeError("Failed to create container-specific directory %s. Error: %v", datadir, err)
	}
	logger.Info("Created container-specific directory %s\n", datadir)

	// Keep track of port mapping
	// We only need to keep track of the mapping of the container's tcp 32262 on the host
	hostPort, exists := c.NetworkSettings.Ports["32262/tcp"]
	if !exists {
		return logger.MakeError("Could not find mapping for port 32262/tcp for container %s with name %s", id, c.Name)
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
		err := logger.MakeError("containerStartHandler(): The hostPort %s for container %s did not parse into a uint16!", hostPort, id)
		return err
	}
	hostPortUint16 := uint16(hostPortInt64)
	containerIDs[hostPortUint16] = id

	// We do not mark the container as "ready" but instead let handleStartValuesRequest
	// do that, since we don't want to mark a container as ready until the start values are
	// set. We are confident that the start values request will arrive after the Docker
	// start handler is called, since the start values request is not triggered until the
	// webserver receives the hostPort from AWS, which does not happen until the
	// container is in a "RUNNING" state.

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
	logger.Info("Flushing Sentry...")
	logger.FlushSentry()

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

// Delete the directory used to store the container resource allocations
// (e.g. TTYs and cloud storage folders) on disk
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
		logger.Panicf("Unable to initialize Sentry. Error: %s", err)
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

			case *httpserver.RegisterDockerContainerIDRequest:
				err := addFractalIDMappings(serverevent.(*httpserver.RegisterDockerContainerIDRequest))
				if err != nil {
					logger.Error(err)
				}
				serverevent.ReturnResult("", err)

			case *httpserver.CreateUinputDevicesRequest:
				list, err := createUinputDevices(serverevent.(*httpserver.CreateUinputDevicesRequest))
				if err != nil {
					logger.Error(err)
					serverevent.ReturnResult("", err)
				} else {
					if result, err := json.Marshal(list); err != nil {
						logger.Error(err)
						serverevent.ReturnResult("", err)
					} else {
						serverevent.ReturnResult(string(result), nil)
					}
				}

			case *httpserver.RequestPortBindingsRequest:
				list, err := allocateHostPorts(serverevent.(*httpserver.RequestPortBindingsRequest))
				if err != nil {
					logger.Error(err)
					serverevent.ReturnResult("", err)
				} else {
					if result, err := json.Marshal(list); err != nil {
						logger.Error(err)
						serverevent.ReturnResult("", err)
					} else {
						serverevent.ReturnResult(string(result), nil)
					}
				}

			default:
				err := logger.MakeError("unimplemented handling of server event [type: %T]: %v", serverevent, serverevent)
				serverevent.ReturnResult("", err)
				logger.Error(err)
			}
		}
	}
}
