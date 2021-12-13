package main

import (
	"github.com/whisthq/whist/core-go/subscriptions"
)

// ScalingAlgorithm is the basic abstraction of the scaling service
// that receives a stream of events and makes calls to the host handler.
type ScalingAlgorithm interface {
	ProcessEvents()
	HandleInstanceEvent(*subscriptions.InstanceEvent) ScalingEvent
	HandleMandelboxEvent(*subscriptions.MandelboxEvent) ScalingEvent
	createEventChans()
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions.
type ScalingEvent struct {
	Name   string
	Type   subscriptions.SubscriptionEvent
	Data   interface{}
	Action string
	Region string
}

// BaseScalingAlgorithm abstracts the shared functionalities to be used
// by all of the different, region-based scaling algorithms.
type BaseScalingAlgorithm struct {
	instanceEventChan  chan *subscriptions.InstanceEvent
	mandelboxEventChan chan *subscriptions.MandelboxEvent
}

// createEventChans instanciates the event channels.
func (s *BaseScalingAlgorithm) createEventChans() {
	if s.instanceEventChan == nil {
		s.instanceEventChan = make(chan *subscriptions.InstanceEvent, 100)
	}

	if s.mandelboxEventChan == nil {
		s.mandelboxEventChan = make(chan *subscriptions.MandelboxEvent, 100)
	}
}

// ProcessEvents is the mainn event loop from the scaling algorithm that
// will redirect the received events to the appropiate channel.
func (s *BaseScalingAlgorithm) ProcessEvents() {
	select {
	case instanceEvent := <-s.instanceEventChan:
		// Create a scaling event
		scalingEvent := s.HandleInstanceEvent(instanceEvent)

		switch scalingEvent.Action {
		// Actually make the scaling decisions with the data
		// from the scaling event
		}

	case mandelboxEvent := <-s.mandelboxEventChan:
		// Create a scaling event
		scalingEvent := s.HandleMandelboxEvent(mandelboxEvent)

		switch scalingEvent.Action {
		// Actually make the scaling decisions with the data
		// from the scaling event
		}

	}
}

// HandleInstanceEvent is responsible of processing an instance event and
// building a scaling event accordingly.
func (s *BaseScalingAlgorithm) HandleInstanceEvent(event *subscriptions.InstanceEvent) ScalingEvent {
	var instance subscriptions.Instance
	if len(event.InstanceInfo) > 0 {
		instance = event.InstanceInfo[0]
	}

	//TODO verify if instance is nil

	var instanceScalingEvent = ScalingEvent{
		Type: event,
	}

	// Instance is ready, add to database and start host service
	if instance.Status == "PRE_CONNECTION" {
		instanceScalingEvent.Action = "MARK_INSTANCE_AS_READY_DATABASE"
	}

	// Instance is draining, scale down?
	if instance.Status == "DRAINING" {
		instanceScalingEvent.Action = "VERIFY_INSTANCE_SCALE_DOWN"
	}

	return instanceScalingEvent
}

// HandleMandelboxEvent is responsible of processing a mandelbox event and
// building a scaling event accordingly.
func (s *BaseScalingAlgorithm) HandleMandelboxEvent(event *subscriptions.MandelboxEvent) ScalingEvent {
	var mandelbox subscriptions.Mandelbox
	if len(event.MandelboxInfo) > 0 {
		mandelbox = event.MandelboxInfo[0]
	}

	//TODO verify if mandelbox is nil

	var mandelboxScalingEvent = ScalingEvent{
		Type: event,
	}

	// Mandelbox was allocated, start instance spin up
	if mandelbox.Status == "ALLOCATED" {
		mandelboxScalingEvent.Action = "SPIN_UP_INSTANCE_FOR_ALLOCATED_MANDELBOX"
	}

	// Mandelbox exited, verify if we need to scale down instance
	if mandelbox.Status == "EXITED" {
		mandelboxScalingEvent.Action = "TRY_SCALE_DOWN_IF_NECESSARY"
	}

	return mandelboxScalingEvent
}

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
