package main

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/core-go/subscriptions"
	logger "github.com/whisthq/whist/backend/core-go/whistlogger"
)

func main() {
	globalCtx, globalCancel := context.WithCancel(context.Background())

	go func() {
		<-globalCtx.Done()
		globalCancel()
	}()

	goroutineTracker := &sync.WaitGroup{}
	whistClient := &subscriptions.WhistClient{}

	// Start database subscriptions
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)
	subscriptions.SetupScalingSubscriptions(whistClient)

	err := subscriptions.Start(whistClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscriptions. Error: %s", err)
	}

	// Start event loop
	go eventLoop(globalCtx, globalCancel, goroutineTracker, subscriptionEvents)
}

func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc,
	goroutineTracker *sync.WaitGroup, subscriptionEvents <-chan subscriptions.SubscriptionEvent) {

	for {
		subscriptionEvent := <-subscriptionEvents
		switch subscriptionEvent := subscriptionEvent.(type) {
		case *subscriptions.MandelboxEvent:
			logger.Infof("%v", subscriptionEvent)
		}
	}
}
