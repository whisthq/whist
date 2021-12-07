package main

import (
	"github.com/fractal/fractal/core-go/subscriptions"
)

type ScalingAlgorithm interface {
	ProcessMandelboxEvent(subscriptions.MandelboxEvent)
	createEventChan() chan ScalingEvent
}

type ScalingEvent struct {
	Name    string
	Details interface{}
}

type USEastScalingAlgorithm struct {
	regionCode string
	eventChan  chan ScalingEvent
}

func (s *USEastScalingAlgorithm) createEventChan() {
	if s.eventChan == nil {
		s.eventChan = make(chan ScalingEvent, 100)
	}
}
