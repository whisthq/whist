package main

import (
	"context"
	"fmt"
	"io"
	"os"
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
	fmt.Println("Checking permissions...")

	if os.Geteuid() != 0 {
		panic("This service needs to run as root!")
	}
}

// Create the directory used to store the container resource allocations (e.g.
// TTYs) on disk
func initializeFilesystem() {
	fmt.Printf("Initializing filesystem in %s\n", resourceMappingDirectory)

	// check if resource mapping directory already exists --- if so, panic, since
	// we don't know why it's there or if it's valid
	if _, err := os.Lstat(resourceMappingDirectory); !os.IsNotExist(err) {
		panic(fmt.Sprintf("%s already exists! Bailing out...\n", resourceMappingDirectory))
	}

	err := os.MkdirAll(resourceMappingDirectory, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create directory %s\n", resourceMappingDirectory)
		panic(err)
	}
}

func uninitializeFilesystem() {
	err := os.RemoveAll(resourceMappingDirectory)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete directory %s\n", resourceMappingDirectory)
		panic(err)
	}
}

func writeAssignmentToFile(filename, data string) {
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Unable to create file %s to store assingment for container", filename)
		panic(err)
	}
	defer file.Sync()
	defer file.Close()
	_, err = file.WriteString(data)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Couldn't write to file %s", filename)
		panic(err)
	} else {
		fmt.Printf("Wrote data %s to file %s\n", data, filename)
	}
}

func containerStartHandler(ctx context.Context, cli *client.Client, id string, ttyState *[256]string) {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + id + "/"
	err := os.Mkdir(datadir, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create container-specific directory %s\n", datadir)
		panic(err)
	} else {
		fmt.Printf("Created container-specific directory %s\n", datadir)
	}

	c, err := cli.ContainerInspect(ctx, id)
	if err != nil {
		panic(err)
	}

	// Keep track of port mapping
	// We only need to keep track of the mapping of the container's tcp 32262 on the host
	host_port, exists := c.NetworkSettings.Ports["32262/tcp"]
	if !exists {
		panic(fmt.Sprintf("Could not find mapping for port 32262/tcp for container %s", id))
	}
	if len(host_port) != 1 {
		panic(fmt.Sprintf("The host_port mapping for port 32262/tcp for container %s has length not equal to 1!. Mapping: %+v", host_port))
	}
	writeAssignmentToFile(datadir+"host_port_for_my_32262_tcp", host_port[0].HostPort)

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
		panic(fmt.Sprintf("Was not able to assign an free tty to container id %s\n", id))
	}

	// Write the tty assignment to a file
	writeAssignmentToFile(datadir+"tty", fmt.Sprintf("%d\n", assigned_tty))

	// Indicate that we are ready for the container to read the data back
	writeAssignmentToFile(datadir+".ready", " ")
}

func containerDieHandler(ctx context.Context, cli *client.Client, id string, ttyState *[256]string) {
	// Delete the container-specific data directory we used
	datadir := resourceMappingDirectory + id + "/"
	err := os.RemoveAll(datadir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete container-specific directory %s\n", datadir)
		panic(err)
	}

	for tty := range ttyState {
		if ttyState[tty] == id {
			ttyState[tty] = ""
		}
	}
}

func main() {
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
		fmt.Println("Got an interrupt or SIGTERM --- calling uninitializeFilesystem() and exiting...")
		uninitializeFilesystem()
		os.Exit(1)
	}()

	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv)
	if err != nil {
		panic(err)
	}

	filters := filters.NewArgs()
	filters.Add("type", events.ContainerEventType)
	eventOptions := types.EventsOptions{
		Filters: filters,
	}

	events, errs := cli.Events(context.Background(), eventOptions)

	// reserve the first 10 TTYs for the host system
	const r = "reserved"
	ttyState := [256]string{r, r, r, r, r, r, r, r, r, r}

loop:
	for {
		select {
		case err := <-errs:
			if err != nil && err != io.EOF {
				panic(err)
			}
			break loop
		case event := <-events:
			if event.Action == "die" {
				containerDieHandler(ctx, cli, event.ID, &ttyState)
			}
			if event.Action == "start" {
				containerStartHandler(ctx, cli, event.ID, &ttyState)
			}
			if event.Action == "die" || event.Action == "start" {
				fmt.Printf("%s %s %s\n", event.Type, event.ID, event.Action)
				for tty, id := range ttyState {
					if id != "" {
						fmt.Printf(" %2d | %s\n", tty, id)
					}
				}
			}
		}
	}
}
