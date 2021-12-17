package main

import (
	"context"
	"os"
	"os/signal"
	"sync"
	"syscall"

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

	// Start database subscription client
	subscriptionClient := &subscriptions.SubscriptionClient{}
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)

	subscriptions.SetupScalingSubscriptions(subscriptionClient)

	err := subscriptions.Start(subscriptionClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscription client. Error: %s", err)
	}

	// Start GraphQL client
	graphqlClient := &subscriptions.GraphQLClient{}

	err = graphqlClient.Initialize()
	if err != nil {
		logger.Errorf("Failed to start GraphQL client. Error: %v", err)
	}

	// algorithmByRegionMap holds all of the scaling algorithms mapped by region.
	// Use a sync map since we only write the keys once but will be reading multiple
	// times by different goroutines.
	algorithmByRegionMap := sync.Map{}
	algorithmByRegionMap.Store("us-east-1", &sa.DefaultScalingAlgorithm{
		Region: "us-east",
	})

	// Instantiate scaling algorithms on allowed regions
	algorithmByRegionMap.Range(func(key, value interface{}) bool {
		scalingAlgorithm := value.(sa.ScalingAlgorithm)
		scalingAlgorithm.CreateBuffer()
		scalingAlgorithm.CreateEventChans()
		scalingAlgorithm.CreateGraphQLClient(graphqlClient)
		scalingAlgorithm.ProcessEvents(goroutineTracker)

		return true
	})

	// Start main event loop
	go eventLoop(globalCtx, globalCancel, goroutineTracker, subscriptionEvents, algorithmByRegionMap)

	// Register a signal handler for Ctrl-C so that we cleanup if Ctrl-C is pressed.
	sigChan := make(chan os.Signal, 2)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)

	// Wait for either the global context to get cancelled by a worker goroutine,
	// or for us to receive an interrupt. This needs to be the end of main().
	select {
	case <-sigChan:
		logger.Infof("Got an interrupt or SIGTERM")
	case <-globalCtx.Done():
		logger.Infof("Global context cancelled!")
	}
}

func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup,
	subscriptionEvents <-chan subscriptions.SubscriptionEvent, algorithmByRegion sync.Map) {

	for {
		select {
		case subscriptionEvent := <-subscriptionEvents:
			// Since we are dealing with a database event, figure out what type it has
			// and send it to the corresponding channel
			switch subscriptionEvent := subscriptionEvent.(type) {

			case *subscriptions.InstanceEvent:
				var scalingEvent sa.ScalingEvent

				scalingEvent.Type = "INSTANCE_DATABASE_EVENT"

				if len(subscriptionEvent.InstanceInfo) > 0 {
					instance := subscriptionEvent.InstanceInfo[0]

					scalingEvent.Data = instance
					scalingEvent.Region = instance.Location
				}

				// Start scaling algorithm based on region
				logger.Infof("Received database event.")

				algorithm, ok := algorithmByRegion.Load(scalingEvent.Region)
				if !ok {
					logger.Errorf("%v not found on algorithm map", scalingEvent.Region)
				}
				scalingAlgorithm := algorithm.(*sa.DefaultScalingAlgorithm)

				logger.Infof("Sending to instance event chan")
				scalingAlgorithm.InstanceEventChan <- scalingEvent

			case *subscriptions.MandelboxEvent:
			}
		}
	}
}
