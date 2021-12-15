package scaling_algorithms

import (
	"sync"

	"github.com/whisthq/whist/core-go/subscriptions"
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
type ScalingEvent struct {
	Type   interface{} // The type of event (database, timing, etc.)
	Data   interface{} // Data relevant to the event
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

func (s *BaseScalingAlgorithm) GetInstanceEventChan() chan ScalingEvent {
	return s.instanceEventChan
}

func (s *BaseScalingAlgorithm) GetMandelboxEventChan() chan ScalingEvent {
	return s.mandelboxEventChan
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
