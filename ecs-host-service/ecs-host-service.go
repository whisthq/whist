package main

import (
	"context"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"syscall"

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
	log.Println("Checking permissions...")

	if os.Geteuid() != 0 {
		log.Panic("This service needs to run as root!")
	}
}

func startDockerDaemon() {
	log.Println("Starting docker daemon ourselves...")
	cmd := exec.Command("/usr/bin/systemctl", "start", "docker")
	err := cmd.Run()
	if err != nil {
		log.Panicf("Unable to start Docker daemon. Error: %v", err)
	} else {
		log.Println("Successfully started the Docker daemon ourselves.")
	}
}

func shutdownHostService() {
	// Catch any panics in the main goroutine. Note that besides the host machine
	// itself shutting down, this method should be the _only_ way that this host
	// service exits. In particular, we use panic() as a control flow primitive
	// --- panics in main() or its child functions will be recovered here and
	// used as signals to exit. Therefore, panics should only be used/bubble up
	// to main() in the case of an irrecoverable failure that mandates that the
	// host machine accept no new connections.
}

// Create the directory used to store the container resource allocations (e.g.
// TTYs) on disk
func initializeFilesystem() {
	log.Printf("Initializing filesystem in %s\n", resourceMappingDirectory)

	// check if resource mapping directory already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(resourceMappingDirectory); !os.IsNotExist(err) {
		if err == nil {
			log.Panicf("Directory %s already exists!", resourceMappingDirectory)
		} else {
			log.Panicf("Could not make directory %s because of error %v", resourceMappingDirectory, err)
		}
	}

	err := os.MkdirAll(resourceMappingDirectory, 0644|os.ModeSticky)
	if err != nil {
		log.Panicf("Failed to create directory %s: error: %s\n", resourceMappingDirectory, err)
	}
}

func uninitializeFilesystem() {
	err := os.RemoveAll(resourceMappingDirectory)
	if err != nil {
		log.Panicf("Failed to delete directory %s: error: %v\n", resourceMappingDirectory, err)
	} else {
		log.Printf("Successfully deleted directory %s\n", resourceMappingDirectory)
	}
}

func writeAssignmentToFile(filename, data string) error {
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644|os.ModeSticky)
	if err != nil {
		err := fmt.Errorf("Unable to create file %s to store resource assignment. Error: %v", filename, err)
		return err
	}
	defer file.Sync()
	defer file.Close()

	_, err = file.WriteString(data)
	if err != nil {
		err := fmt.Errorf("Couldn't write assignment with data %s to file %s. Error: %v", data, filename, err)
		return err
	} else {
		log.Printf("Wrote data \"%s\" to file %s\n", data, filename)
		return nil
	}
}

func containerStartHandler(ctx context.Context, cli *client.Client, id string, ttyState *[256]string) error {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + id + "/"
	err := os.Mkdir(datadir, 0644|os.ModeSticky)
	if err != nil {
		err := fmt.Errorf("Failed to create container-specific directory %s. Error: %v\n", datadir, err)
		return err
	} else {
		log.Printf("Created container-specific directory %s\n", datadir)
	}

	c, err := cli.ContainerInspect(ctx, id)
	if err != nil {
		err := fmt.Errorf("Error running ContainerInspect on container %s: %v\n", id, err)
		return err
	}

	// Keep track of port mapping
	// We only need to keep track of the mapping of the container's tcp 32262 on the host
	host_port, exists := c.NetworkSettings.Ports["32262/tcp"]
	if !exists {
		err := fmt.Errorf("Could not find mapping for port 32262/tcp for container %s", id)
		return err
	}
	if len(host_port) != 1 {
		err := fmt.Errorf("The host_port mapping for port 32262/tcp for container %s has length not equal to 1!. Mapping: %+v", id, host_port)
		return err
	}
	err = writeAssignmentToFile(datadir+"host_port_for_my_32262_tcp", host_port[0].HostPort)
	if err != nil {
		// Don't need to wrap err here because writeAssignmentToFile already contains the relevant info
		return err
	}

	// Assign an unused tty
	assigned_tty := -1
	for tty := range ttyState {
		if ttyState[tty] == "" {
			assigned_tty = tty
			ttyState[assigned_tty] = id
			break
		}
	}
	if assigned_tty == -1 {
		err := fmt.Errorf("Was not able to assign an free tty to container id %s\n", id)
		return err
	}

	// Write the tty assignment to a file
	err = writeAssignmentToFile(datadir+"tty", fmt.Sprintf("%d", assigned_tty))
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
		err := fmt.Errorf("Failed to delete container-specific directory %s\n", datadir)
		return err
	} else {
		fmt.Printf("Successfully deleted container-specific directory %s\n", datadir)
	}

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

	// After the above defer, this needs to be the next line of code that runs,
	// since the following operations will require root permissions.
	checkRunningPermissions()

	// Note that we defer uninitialization so that in case of panic elsewhere, we
	// still clean up
	initializeFilesystem()
	defer uninitializeFilesystem()

	// Register a signal handler for Ctrl-C so that we still cleanup if Ctrl-C is pressed
	sig_chan := make(chan os.Signal, 2)
	signal.Notify(sig_chan, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-sig_chan
		log.Println("Got an interrupt or SIGTERM --- calling uninitializeFilesystem() and panicking...")
		uninitializeFilesystem()
		log.Panic("Got a Ctrl+C: already uninitialized filesystem, looking to exit")
	}()

	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv)
	if err != nil {
		log.Panic(err)
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
	log.Println("Initialized event stream...")

	for {
		if needToReinitializeEventStream {
			events, errs = cli.Events(context.Background(), eventOptions)
			needToReinitializeEventStream = false
			log.Println("Re-initialized event stream...")
		}

		select {
		case err := <-errs:
			needToReinitializeEventStream = true
			switch {
			case err == nil:
				log.Println("We got a nil error over the Docker event stream. This might indicate an error inside the docker go library. Ignoring it and proceeding normally...")
				continue
			case err == io.EOF:
				log.Panic("Docker event stream has been completely read.")
				break
			case client.IsErrConnectionFailed(err):
				// This means "Cannot connect to the Docker daemon..."
				log.Printf("Got error \"%v\". Trying to start Docker daemon ourselves...", err)
				startDockerDaemon()
				continue
			default:
				err := fmt.Errorf("Got an unknown error from the Docker event stream: %v", err)
				log.Panic(err)
			}

		case event := <-events:
			if event.Action == "die" || event.Action == "start" {
				log.Printf("Event: %s for %s %s\n", event.Action, event.Type, event.ID)
			}
			if event.Action == "die" {
				err := containerDieHandler(ctx, cli, event.ID, &ttyState)
				if err != nil {
					log.Panic(err)
				}
			}
			if event.Action == "start" {
				err := containerStartHandler(ctx, cli, event.ID, &ttyState)
				if err != nil {
					log.Panic(err)
				}
			}
		}
	}
	// Should be unreachable!
	log.Println("Reached the end of main()...")
}
