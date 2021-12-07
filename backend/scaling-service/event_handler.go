package main

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/core-go/subscriptions"
	logger "github.com/whisthq/whist/backend/core-go/whistlogger"
)

func main() {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := &sync.WaitGroup{}

	// goroutine that fires when the global context is canceled.
	go func() {
		<-globalCtx.Done()
	}()

	// Start database subscriptions
	whistClient := &subscriptions.WhistClient{}
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)

	subscriptions.SetupScalingSubscriptions(whistClient)

	err := subscriptions.Start(whistClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscriptions. Error: %s", err)
	}

	// Start main event loop
	go eventLoop(globalCtx, globalCancel, goroutineTracker, subscriptionEvents)
}

func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup,
	subscriptionEvents <-chan subscriptions.SubscriptionEvent) {

	for {
		subscriptionEvent := <-subscriptionEvents
		switch subscriptionEvent := subscriptionEvent.(type) {
		case *subscriptions.MandelboxEvent:
			// Start scaling algorithm based on region
		}
	}
}
