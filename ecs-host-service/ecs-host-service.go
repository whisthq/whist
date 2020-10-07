package main

import (
	"context"
	"fmt"
	"io"
	"os"

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
	fmt.Println("Initializing filesystem in %s", resourceMappingDirectory)

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
	}
}

func containerStartHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[256]string) error {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + id + "/"
	err := os.Mkdir(datadir, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create container-specific directory %s\n", datadir)
		panic(err)
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
		panic(fmt.Sprintf("Was not able to assign an free tty to container id %s\n", id))
	}

	// Write the tty assignment to a file
	writeAssignmentToFile(datadir+"tty", fmt.Sprintf("%d\n", assigned_tty))

	return nil
}

func containerStopHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[256]string) error {
	// Delete the container-specific data directory we used
	datadir := resourceMappingDirectory + id + "/"
	err := os.RemoveAll(datadir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete container-specific directory %s\n", datadir)
		panic(err)
	}

	// Delete the tty assignment file and mark it as unused
	if err := os.Remove(datadir + "tty"); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete container-specific tty assignment file %s\n", datadir+"tty")
		panic(err)
	}
	for tty := range ttyState {
		if ttyState[tty] == id {
			ttyState[tty] = ""
		}
	}
	// delete(portMap, id)
	return nil
}

func main() {
	checkRunningPermissions()

	// Note that we defer uninitialization so that in case of panic elsewhere, we
	// still clean up
	initializeFilesystem()
	defer uninitializeFilesystem()

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
	portMap := make(map[string]map[string]string)

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
			if event.Action == "stop" {
				containerStopHandler(ctx, cli, event.ID, portMap, &ttyState)
			}
			if event.Action == "start" {
				containerStartHandler(ctx, cli, event.ID, portMap, &ttyState)
			}
			if event.Action == "stop" || event.Action == "start" {
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
