package main

import (
	"github.com/whisthq/whist/core-go/subscriptions"
	logger "github.com/whisthq/whist/core-go/whistlogger"
	"github.com/whisthq/whist/scaling-service/hosts"
)

// DEFAULT_INSTANCE_BUFFER is the number of instances that should always
// be running on each region.
const DEFAULT_INSTANCE_BUFFER = 1

// ScalingAlgorithm is the basic abstraction of the scaling service
// that receives a stream of events and makes calls to the host handler.
type ScalingAlgorithm interface {
	ProcessEvents()
	CreateBuffer()
	CreateGraphQLClient(*subscriptions.GraphQLClient)
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions.
type ScalingEvent struct {
	Type   interface{} // The type of event (database, timing, etc.)
	Data   interface{} // Data relevant to the event
	Action string      // The scaling action to perform
	Region string      // Region where the scaling will be performed
}

// BaseScalingAlgorithm abstracts the shared functionalities to be used
// by all of the different, region-based scaling algorithms.
type BaseScalingAlgorithm struct {
	Host               hosts.HostHandler
	GraphQLClient      *subscriptions.GraphQLClient
	InstanceBuffer     *int32
	Region             string
	instanceEventChan  chan ScalingEvent
	mandelboxEventChan chan ScalingEvent
}

// CreateEventChans creates the event channels if they don't alredy exist.
func (s *BaseScalingAlgorithm) CreateEventChans() {
	if s.instanceEventChan != nil {
		s.instanceEventChan = make(chan ScalingEvent, 100)
	}
	if s.mandelboxEventChan != nil {
		s.mandelboxEventChan = make(chan ScalingEvent, 100)
	}
}

// CreateBuffer initializes the instance buffer.
func (s *BaseScalingAlgorithm) CreateBuffer() {
	buff := int32(DEFAULT_INSTANCE_BUFFER)
	s.InstanceBuffer = &buff
}

// CreateGraphQLClient sets the graphqlClient to be used on queries.
func (s *BaseScalingAlgorithm) CreateGraphQLClient(client *subscriptions.GraphQLClient) {
	if s.GraphQLClient == nil {
		s.GraphQLClient = client
	}
}

// ProcessEvents is the main function of the scaling algorithm, it is
// reponsible of processing events and executing the appropiate scaling actions.
func (s *BaseScalingAlgorithm) ProcessEvents() {
	if s.Host == nil {
		// TODO when multi-cloud support is introduced, figure out a way to
		// decide which host to use. For now default to AWS.
		handler := &hosts.AWSHost{}
		err := handler.Initialize(s.Region)

		if err != nil {
			logger.Errorf("Error starting host on USEast. Error: %v", err)
		}

		s.Host = handler
	}

	logger.Infof("Scaling algorithm listening for events...")
	for {
		select {
		case instanceEvent := <-s.instanceEventChan:
			instance := instanceEvent.Data.(subscriptions.Instance)

			if instance.Status == "PRE_CONNECTION" {

			}

			if instance.Status == "DRAINING" {

			}

		case mandelboxEvent := <-s.mandelboxEventChan:
			mandelbox := mandelboxEvent.Data.(subscriptions.Mandelbox)

			if mandelbox.Status == "ALLOCATED" {

			}
		}
	}
}

// Each of the following region based scaling algorithms are to be used when
// the need for specialized rules for each regionn arises.
// USEastScalingAlgorithm is a type to make scaling decisions for the US East region.
type USEastScalingAlgorithm struct {
	*BaseScalingAlgorithm
}

// USWestScalingAlgorithm is a type to make scaling decisions for the US West region.
type USWestScalingAlgorithm struct {
	*BaseScalingAlgorithm
}

// CACentralScalingAlgorithm is a type to make scaling decisions for the Canada Centeral region.
type CACentralScalingAlgorithm struct {
	*BaseScalingAlgorithm
}
