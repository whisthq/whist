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
const resourceMappingDirectory = "/fractal/containerResourceMappings"

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
		panic(fmt.Sprintf("%s already exists! Bailing out...", resourceMappingDirectory))
	}

	err := os.MkdirAll(resourceMappingDirectory, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create directory %s", resourceMappingDirectory)
		panic(err)
	}
}

func uninitializeFilesystem() {
	err := os.RemoveAll(resourceMappingDirectory)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete directory %s", resourceMappingDirectory)
		panic(err)
	}
}

func containerStartHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[256]string) error {
	// Create a container-specific directory to store mappings
	datadir := resourceMappingDirectory + "/" + id
	err := os.Mkdir(datadir, 0644|os.ModeSticky)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create container-specific directory %s", datadir)
		panic(err)
	}

	// Assign an unused tty
	for tty := range ttyState {
		if ttyState[tty] == "" {
			ttyState[tty] = id

			// write to fs to add pair (tty, id)
			break
		}
	}

	return nil
}

func containerStopHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[256]string) error {
	// Delete the container-specific data directory we used
	datadir := resourceMappingDirectory + "/" + id
	err := os.RemoveAll(datadir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to delete container-specific directory %s", datadir)
		panic(err)
	}

	for tty := range ttyState {
		if ttyState[tty] == id {
			ttyState[tty] = ""
			// write to fs to remove pair (tty, id)
			break
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
