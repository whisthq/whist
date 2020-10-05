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

func main() {
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

	loop:
		for {
			select {
			case err := <-errs:
				if err != nil && err != io.EOF {
					panic(err)
				}
				break loop
			case event := <-events:
				fmt.Printf("%s %s %s\n", event.Type, event.ID, event.Action)
			}
		}
}
