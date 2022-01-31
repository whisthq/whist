package main

import (
	"context"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/go-co-op/gocron"
	sa "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default" // Import as sa, short for scaling_algorithms
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

func main() {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := &sync.WaitGroup{}

	// goroutine that fires when the global context is canceled.
	go func() {
		<-globalCtx.Done()
	}()

	// Start GraphQL client for queries/mutations
	graphqlClient := &subscriptions.GraphQLClient{}
	err := graphqlClient.Initialize()
	if err != nil {
		logger.Errorf("Failed to start GraphQL client. Error: %v", err)
	}

	// Start database subscriptions
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)
	StartDatabaseSubscriptions(globalCtx, goroutineTracker, subscriptionEvents)

	// Start scheduler and setup scheduler event chan
	scheduledEvents := make(chan sa.ScalingEvent, 100)
	StartSchedulerEvents(scheduledEvents)

	// Start the deploy events once since we are starting the scaling service.
	StartDeploy(scheduledEvents)

	// algorithmByRegionMap holds all of the scaling algorithms mapped by region.
	// Use a sync map since we only write the keys once but will be reading multiple
	// times by different goroutines.
	algorithmByRegionMap := &sync.Map{}

	// Load default scaling algorithm for all enabled regions.
	for _, region := range sa.BundledRegions {
		name := utils.Sprintf("default-sa-%s", region)
		algorithmByRegionMap.Store(name, &sa.DefaultScalingAlgorithm{
			Region: region,
		})
	}

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
	go eventLoop(globalCtx, globalCancel, goroutineTracker, subscriptionEvents, scheduledEvents, algorithmByRegionMap)

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

// StartDatabaseSubscriptions sets up the database subscriptions and starts the subscription client.
func StartDatabaseSubscriptions(globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan subscriptions.SubscriptionEvent) {
	subscriptionClient := &subscriptions.SubscriptionClient{}
	subscriptions.SetupScalingSubscriptions(subscriptionClient)

	err := subscriptions.Start(subscriptionClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscription client. Error: %s", err)
	}

}

// StartSchedulerEvents starts the scheduler and its events without blocking the main thread.
func StartSchedulerEvents(scheduledEvents chan sa.ScalingEvent) {
	s := gocron.NewScheduler(time.UTC)

	// Schedule scale down routine every 10 minutes, start 10 minutes from now.
	t := time.Now().Add(10 * time.Minute)
	s.Every(10).Minutes().StartAt(t).Do(func() {
		// Send to scheduling channel
		scheduledEvents <- sa.ScalingEvent{
			Type: "SCHEDULED_SCALE_DOWN",
		}
	})

	s.StartAsync()
}

func StartDeploy(scheduledEvents chan sa.ScalingEvent) {
	// Get arguments
	regionImageMap := os.Args[1:]

	// Send image upgrade event to scheduled chan.
	scheduledEvents <- sa.ScalingEvent{
		Type: "SCHEDULED_IMAGE_UPGRADE",
		Data: regionImageMap,
	}
}

// getScalingAlgorithm is a helper function that returns the scaling algorithm from the sync map.
func getScalingAlgorithm(algorithmByRegion *sync.Map, scalingEvent sa.ScalingEvent) sa.ScalingAlgorithm {
	// Try to get the scaling algorithm on the region the scaling event was requested.
	// TODO: figure out how to get non-default scaling algorihtms.
	name := utils.Sprintf("default-sa-%s", scalingEvent.Region)
	algorithm, ok := algorithmByRegion.Load(name)
	if ok {
		return algorithm.(sa.ScalingAlgorithm)
	}

	logger.Errorf("Failed to get scaling algorithm in %v", scalingEvent.Region)

	return nil
}

// eventLoop is the main loop of the scaling service which will receive events from different sources
// and send them to the appropiate channels.
func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup,
	subscriptionEvents <-chan subscriptions.SubscriptionEvent, scheduledEvents <-chan sa.ScalingEvent, algorithmByRegion *sync.Map) {

	for {
		select {
		case subscriptionEvent := <-subscriptionEvents:
			// Since we are dealing with a database event, figure out what type it has
			// and send it to the corresponding channel
			switch subscriptionEvent := subscriptionEvent.(type) {

			case *subscriptions.InstanceEvent:
				var scalingEvent sa.ScalingEvent

				scalingEvent.Type = "INSTANCE_DATABASE_EVENT"

				if len(subscriptionEvent.Instances) > 0 {
					instance := subscriptionEvent.Instances[0]

					scalingEvent.Data = instance
					scalingEvent.Region = instance.Region
				}

				// Start scaling algorithm based on region
				logger.Infof("Received database event.")
				algorithm := getScalingAlgorithm(algorithmByRegion, scalingEvent)

				switch algorithm := algorithm.(type) {
				case *sa.DefaultScalingAlgorithm:
					algorithm.InstanceEventChan <- scalingEvent
				}
			}
		case scheduledEvent := <-scheduledEvents:
			// Start scaling algorithm based on region
			logger.Infof("Received scheduled event. %v", scheduledEvent)

			for _, region := range sa.BundledRegions {
				scheduledEvent.Region = region
				algorithm := getScalingAlgorithm(algorithmByRegion, scheduledEvent)
				switch algorithm := algorithm.(type) {
				case *sa.DefaultScalingAlgorithm:
					logger.Infof("Sending to scheduled event chan")
					algorithm.ScheduledEventChan <- scheduledEvent
				}
			}
		}
	}
}
