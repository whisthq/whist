// Copyright (c) 2022-2023 Whist Technologies, Inc.

/*
Package scaling_algorithms includes the implementations of scaling algorithms used by
the scaling service. These are the implemented algorithms as of now:

- Default scaling algorithm:
	It handles the scaling of instances by computing mandelbox capacity for each instance
	and determining the overall availability on each region according to a "buffer" set by
	the config database.
*/

package scaling_algorithms

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// ScalingAlgorithm is the basic abstraction of the scaling service
// that receives a stream of events and makes calls to the host handler.
type ScalingAlgorithm interface {
	ProcessEvents(context.Context, *sync.WaitGroup)
	CreateEventChans()
	CreateGraphQLClient(subscriptions.WhistGraphQLClient)
	CreateDBClient(dbclient.WhistDBClient)
	GetConfig(subscriptions.WhistGraphQLClient)
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions.
type ScalingEvent struct {
	ID     string
	Type   interface{} // The type of event (database, timing, etc.)
	Data   interface{} // Data relevant to the event
	Region string      // Region where the scaling will be performed
}
