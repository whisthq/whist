// Copyright (c) 2021-2022 Whist Technologies, Inc.

/*
Package algorithms includes the implementations of all the scaling algorithms used by
the scaling service.

A scaling algorithm is defined as struct that implements the `ScalingAlgorithm` interface
and has a collection of methods that make decisions about how and when to scale instances
in order satisfy demand. Alternative implementations or use cases for a scaling algorithm
could be: a region that has different characteristics such as less instance availability
or user demand, deploying a smarter algorithm on a region with high demand, using different
strategies for detecting user demand, or in general any situation that requires a different
behaviour than the one in the default scaling algorithm.


The currently implemented algorithms are:

- Default scaling algorithm:

	It handles the scaling of instances by computing mandelbox capacity for each instance
	and determining the overall availability on each region according to a "buffer" set by
	the config database.
*/

package algorithms

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// ScalingAlgorithm is the basic abstraction of a scaling algorithm.
// It doesn't make any assumptions about the underlying implementation,
// it only defines the basic access methods that the event handler needs.
type ScalingAlgorithm interface {
	ProcessEvents(context.Context, *sync.WaitGroup)
	CreateEventChans()
	CreateGraphQLClient(subscriptions.WhistGraphQLClient)
	CreateDBClient(dbclient.WhistDBClient)
	GetConfig(subscriptions.WhistGraphQLClient)
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions. Its the common object used throughout the
// scaling service and passed across the different layers.
type ScalingEvent struct {
	// A unique identifier for each scaling event.
	ID string
	// The source where the event originated (database, timing, etc.)
	Type interface{}
	// Data relevant to the event.
	Data interface{}
	// Availability region where the scaling will be performed.
	Region string
}
