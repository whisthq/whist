package scaling_algorithms

import (
	"context"
	"sync"

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
	ProcessEvents(*sync.WaitGroup)
	CreateEventChans()
	CreateBuffer()
	CreateGraphQLClient(*subscriptions.GraphQLClient)
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions.
// Idea: We could use UUIDs for each event so we can improve our logging
// and debugging capabilities.
type ScalingEvent struct {
	Type   interface{} // The type of event (database, timing, etc.)
	Data   interface{} // Data relevant to the event
	Region string      // Region where the scaling will be performed
}

// DefaultScalingAlgorithm abstracts the shared functionalities to be used
// by all of the different, region-based scaling algorithms.
type DefaultScalingAlgorithm struct {
	Host               hosts.HostHandler
	GraphQLClient      *subscriptions.GraphQLClient
	InstanceBuffer     *int32
	Region             string
	InstanceEventChan  chan ScalingEvent
	MandelboxEventChan chan ScalingEvent
}

// CreateEventChans creates the event channels if they don't alredy exist.
func (s *DefaultScalingAlgorithm) CreateEventChans() {
	if s.InstanceEventChan == nil {
		s.InstanceEventChan = make(chan ScalingEvent, 100)
	}
	if s.MandelboxEventChan == nil {
		s.MandelboxEventChan = make(chan ScalingEvent, 100)
	}
}

// CreateBuffer initializes the instance buffer.
func (s *DefaultScalingAlgorithm) CreateBuffer() {
	buff := int32(DEFAULT_INSTANCE_BUFFER)
	s.InstanceBuffer = &buff
}

// CreateGraphQLClient sets the graphqlClient to be used on queries.
func (s *DefaultScalingAlgorithm) CreateGraphQLClient(client *subscriptions.GraphQLClient) {
	if s.GraphQLClient == nil {
		s.GraphQLClient = client
	}
}

// ProcessEvents is the main function of the scaling algorithm, it is responsible of processing
// events and executing the appropiate scaling actions. This function is specific for each region
// scaling algorithm to be able to implement different strategies on each region.
func (s *DefaultScalingAlgorithm) ProcessEvents(goroutineTracker *sync.WaitGroup) {
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

	// Start algorithm main event loop
	goroutineTracker.Add(1)
	go func() {

		for {
			logger.Infof("Scaling algorithm listening for events...")

			select {
			case instanceEvent := <-s.InstanceEventChan:
				logger.Infof("Scaling algorithm received an instance database event with value: %v", instanceEvent)
				instance := instanceEvent.Data.(subscriptions.Instance)

				if instance.Status == "DRAINING" {
					// Create context for scaling operation
					scalingCtx, scalingCancel := context.WithCancel(context.Background())

					err := s.VerifyInstanceScaleDown(scalingCtx, s.Host, instanceEvent, instance)

					// Cancel context once the operation is done
					scalingCancel()

					if err != nil {
						logger.Errorf("Error verifying instance scale down. Error: %v", err)
					}
				}

			case mandelboxEvent := <-s.MandelboxEventChan:
				mandelbox := mandelboxEvent.Data.(subscriptions.Mandelbox)

				if mandelbox.Status == "ALLOCATED" {

				}
			}
		}
	}()

}
