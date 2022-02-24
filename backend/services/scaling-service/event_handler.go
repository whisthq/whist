package main

import (
	"context"
	"encoding/json"
	"os"
	"os/signal"
	"path"
	"sync"
	"syscall"
	"time"

	"github.com/go-co-op/gocron"
	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	algos "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default" // Import as algos, short for scaling_algorithms
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

func main() {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := &sync.WaitGroup{}

	// Start Sentry and Logzio
	logger.InitScalingLogging()

	// Start GraphQL client for queries/mutations
	graphqlClient := &subscriptions.GraphQLClient{}
	err := graphqlClient.Initialize()
	if err != nil {
		logger.Errorf("Failed to start GraphQL client. Error: %v", err)
	}

	// Start database subscriptions
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent, 100)
	subscriptionClient := &subscriptions.SubscriptionClient{}
	configClient := &subscriptions.SubscriptionClient{}
	dbclient := &dbclient.DBClient{}
	StartDatabaseSubscriptions(globalCtx, goroutineTracker, subscriptionEvents, subscriptionClient, configClient)

	// Start scheduler and setup scheduler event chan
	scheduledEvents := make(chan algos.ScalingEvent, 100)

	// Set to run every 10 minutes, starting 10 minutes from now
	start := time.Duration(10 * time.Minute)
	StartSchedulerEvents(scheduledEvents, 10, start)

	// Start the deploy events once since we are starting the scaling service.
	StartDeploy(scheduledEvents)

	// algorithmByRegionMap holds all of the scaling algorithms mapped by region.
	// Use a sync map since we only write the keys once but will be reading multiple
	// times by different goroutines.
	algorithmByRegionMap := &sync.Map{}

	// Load default scaling algorithm for all enabled regions.
	for _, region := range algos.BundledRegions {
		name := utils.Sprintf("default-sa-%s", region)
		algorithmByRegionMap.Store(name, &algos.DefaultScalingAlgorithm{
			Region: region,
		})
	}

	// Instantiate scaling algorithms on allowed regions
	algorithmByRegionMap.Range(func(key, value interface{}) bool {
		scalingAlgorithm := value.(algos.ScalingAlgorithm)
		scalingAlgorithm.CreateEventChans()
		scalingAlgorithm.CreateGraphQLClient(graphqlClient)
		scalingAlgorithm.CreateDBClient(dbclient)
		scalingAlgorithm.ProcessEvents(globalCtx, goroutineTracker)

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
func StartDatabaseSubscriptions(globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan subscriptions.SubscriptionEvent, subscriptionClient subscriptions.WhistSubscriptionClient, configClient subscriptions.WhistSubscriptionClient) {
	subscriptions.SetupScalingSubscriptions(subscriptionClient)
	subscriptions.SetupConfigSubscriptions(configClient)

	// Setup and start subscriptions to main database
	subscriptions.SetupScalingSubscriptions(subscriptionClient)
	err := subscriptions.Start(subscriptionClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscription client. Error: %s", err)
	}

	// Setup and start subscriptions to config database
	subscriptions.SetupScalingSubscriptions(configClient)
	err = subscriptions.Start(configClient, globalCtx, goroutineTracker, subscriptionEvents)
	if err != nil {
		logger.Errorf("Failed to start database subscription client. Error: %s", err)
	}
}

// StartSchedulerEvents starts the scheduler and its events without blocking the main thread.
// `interval` sets the time when the event will run in minutes (i.e. every 10 minutes), and `start`
// sets the time when the first event will happen (i.e. 10 minutes from now).
func StartSchedulerEvents(scheduledEvents chan algos.ScalingEvent, interval interface{}, start time.Duration) {
	s := gocron.NewScheduler(time.UTC)

	// Schedule scale down routine every 10 minutes, start 10 minutes from now.
	t := time.Now().Add(start)
	s.Every(interval).Minutes().StartAt(t).Do(func() {
		// Send into scheduling channel
		scheduledEvents <- algos.ScalingEvent{
			// Create a UUID so we can identify and search this event on our logs
			ID: uuid.NewString(),
			// We set the event type to SCHEDULED_SCALE_DOWN_EVENT
			// here so that we have more information about the event.
			// SCHEDULED means the source of the event is the scheduler
			// and SCALE_DOWN means it will perform a scale down action.
			Type: "SCHEDULED_SCALE_DOWN_EVENT",
		}
	})

	s.StartAsync()
}

func StartDeploy(scheduledEvents chan algos.ScalingEvent) {
	var regionImageMap map[string]interface{}

	// Get current working directory to read images file.
	currentWorkingDirectory, err := os.Getwd()
	if err != nil {
		logger.Errorf("Failed to get working directory. Err: %v", err)
		return
	}

	// Read file which contains the region to image on JSON format. This file will
	// be read by the binary generated during deploy, located in the `bin` directory.
	// The file is also generated during deploy and lives in the scaling service directory.
	content, err := os.ReadFile(path.Join(currentWorkingDirectory, "images.json"))
	if err != nil {
		logger.Errorf("Failed to read region to image map from file. Not performing image upgrade. Err: %v", err)
		return
	}

	// Try to unmarshal contents of file into a map
	err = json.Unmarshal(content, &regionImageMap)
	if err != nil {
		logger.Errorf("Failed to unmarshal region to image map. Not performing image upgrade. Err: %v", err)
		return
	}

	// Send image upgrade event to scheduled chan.
	scheduledEvents <- algos.ScalingEvent{
		// Create a UUID so we can identify and search this event on our logs
		ID: uuid.NewString(),
		// We set the event type to SCHEDULED_IMAGE_UPGRADE_EVENT
		// here so that we have more information about the event.
		// SCHEDULED means the source of the event is the scheduler
		// and IMAGE_UPGRADE means it will perform an image upgrade action.
		Type: "SCHEDULED_IMAGE_UPGRADE_EVENT",
		Data: regionImageMap,
	}
}

// getScalingAlgorithm is a helper function that returns the scaling algorithm from the sync map.
func getScalingAlgorithm(algorithmByRegion *sync.Map, scalingEvent algos.ScalingEvent) algos.ScalingAlgorithm {
	// Try to get the scaling algorithm on the region the scaling event was requested.
	// TODO: figure out how to get non-default scaling algorihtms.
	name := utils.Sprintf("default-sa-%s", scalingEvent.Region)
	algorithm, ok := algorithmByRegion.Load(name)
	if ok {
		return algorithm.(algos.ScalingAlgorithm)
	}

	logger.Errorf("Failed to get scaling algorithm in %v", scalingEvent.Region)

	return nil
}

// eventLoop is the main loop of the scaling service which will receive events from different sources
// and send them to the appropiate channels.
func eventLoop(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup,
	subscriptionEvents <-chan subscriptions.SubscriptionEvent, scheduledEvents <-chan algos.ScalingEvent, algorithmByRegion *sync.Map) {

	for {
		select {
		case subscriptionEvent := <-subscriptionEvents:
			// Since we are dealing with a database event, figure out what type it has
			// and send it to the corresponding channel
			switch subscriptionEvent := subscriptionEvent.(type) {

			case *subscriptions.InstanceEvent:
				var scalingEvent algos.ScalingEvent

				// We set the event type to DATABASE_INSTANCE_EVENT
				// here so that we have more information about the event.
				// DATABASE means the source of the event is the database
				// and INSTANCE_EVENT means it will perform an action with
				// the instance information received from the database.
				scalingEvent.Type = "DATABASE_INSTANCE_EVENT"

				if len(subscriptionEvent.Instances) > 0 {
					instance := subscriptionEvent.Instances[0]
					scalingEvent.ID = uuid.NewString()
					scalingEvent.Data = instance
					scalingEvent.Region = instance.Region
				}

				// Start scaling algorithm based on region
				logger.Infof("Received database event.")
				algorithm := getScalingAlgorithm(algorithmByRegion, scalingEvent)

				switch algorithm := algorithm.(type) {
				case *algos.DefaultScalingAlgorithm:
					algorithm.InstanceEventChan <- scalingEvent
				}
			}
		case scheduledEvent := <-scheduledEvents:
			// Start scaling algorithm based on region
			logger.Infof("Received scheduled event. %v", scheduledEvent)

			for _, region := range algos.BundledRegions {
				scheduledEvent.Region = region
				algorithm := getScalingAlgorithm(algorithmByRegion, scheduledEvent)
				switch algorithm := algorithm.(type) {
				case *algos.DefaultScalingAlgorithm:
					algorithm.ScheduledEventChan <- scheduledEvent
				}
			}
		case <-globalCtx.Done():
			logger.Infof("Gloal context has been cancelled, exiting event loop...")
			return
		}
	}
}
