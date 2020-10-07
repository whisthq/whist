package main

import (
	"context"
	"fmt"
	"io"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/events"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/client"
)

func containerStartHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[64]string) error {

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

func containerStopHandler(ctx context.Context, cli *client.Client, id string, portMap map[string]map[string]string, ttyState *[64]string) error {
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
	ttyState := [64]string{r, r, r, r, r, r, r, r, r, r}
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
