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

	// algorithmByRegion holds all of the scaling algorithms mapped by region.
	// Use a sync map since we only write the keys once but will be reading multiple
	// times by different goroutines.
	algorithmByRegion := sync.Map{}
	algorithmByRegion.Store("us-east", USEastScalingAlgorithm{})
	algorithmByRegion.Store("us-west", USWestScalingAlgorithm{})
	algorithmByRegion.Store("ca-central", CACentralScalingAlgorithm{})

	// Instantiate scaling algorithms on allowed regions
	algorithmByRegion.Range(func(key, value interface{}) bool {
		algorithm := value.(ScalingAlgorithm)
		algorithm.createEventChans()
		algorithm.ProcessEvents()
		return true
	})

	// Start main event loop
	go eventLoop(globalCtx, globalCancel, goroutineTracker, subscriptionEvents, algorithmByRegion)
}

func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup,
	subscriptionEvents <-chan subscriptions.SubscriptionEvent, algorithmByRegion sync.Map) {

	for {
		subscriptionEvent := <-subscriptionEvents
		switch subscriptionEvent := subscriptionEvent.(type) {
		case *subscriptions.MandelboxEvent:
			// Start scaling algorithm based on region
			// Read region from subscription, for this we need to add a region field to the db
			algorithm, ok := algorithmByRegion.Load("us-east")

			if !ok {
				logger.Errorf("USEastScalingAlgorithm not found on algorithm map")
			}

			USEast := algorithm.(USEastScalingAlgorithm)
			USEast.mandelboxEventChan <- subscriptionEvent

		case *subscriptions.InstanceEvent:
			// Instance event
		}
	}
}
