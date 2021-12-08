/*
Package subscriptions is responsible for implementing a pubsub architecture
on the host-service. This is achieved using Hasura live subscriptions, so that
the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/whist/core-go/subscriptions"

import (
	"context"
	"sync"

	"github.com/fractal/whist/core-go/metadata"
	logger "github.com/fractal/whist/core-go/whistlogger"
	graphql "github.com/hasura/go-graphql-client" // We use hasura's own graphql client for Go
)

// InstanceStatusHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func InstanceStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(InstanceEvent)

	var instance Instance
	if len(result.InstanceInfo) > 0 {
		instance = result.InstanceInfo[0]
	}

	instanceName := string(variables["instance_name"].(graphql.String))
	status := string(variables["status"].(graphql.String))

	return (instance.InstanceName == instanceName) && (instance.Status == status)
}

// MandelboxAllocatedHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxAllocatedHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(MandelboxEvent)

	var mandelbox Mandelbox
	if len(result.MandelboxInfo) > 0 {
		mandelbox = result.MandelboxInfo[0]
	}

	instanceName := string(variables["instance_name"].(graphql.String))

	return mandelbox.InstanceName == instanceName
}

// MandelboxStatusHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(MandelboxEvent)

	var mandelbox Mandelbox
	if len(result.MandelboxInfo) > 0 {
		mandelbox = result.MandelboxInfo[0]
	}

	status := string(variables["status"].(graphql.String))

	return mandelbox.Status == status
}

// SetupHostSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the host service.
func SetupHostSubscriptions(instanceName string, whistClient WhistHasuraClient) {
	hostSubscriptions := []HasuraSubscription{
		{
			Query: InstanceStatusQuery,
			Variables: map[string]interface{}{
				"instance_name": graphql.String(instanceName),
				"status":        graphql.String("DRAINING"),
			},
			Result:  InstanceEvent{[]Instance{}},
			Handler: InstanceStatusHandler,
		},
		{
			Query: MandelboxAllocatedQuery,
			Variables: map[string]interface{}{
				"instance_name": graphql.String(instanceName),
				"status":        graphql.String("ALLOCATED"),
			},
			Result:  MandelboxEvent{[]Mandelbox{}},
			Handler: MandelboxAllocatedHandler,
		},
	}
	whistClient.SetSubscriptions(hostSubscriptions)
}

// SetupScalingSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the scaling service.
func SetupScalingSubscriptions(whistClient WhistHasuraClient) {
	scalingSubscriptions := []HasuraSubscription{
		{
			Query: MandelboxStatusQuery,
			Variables: map[string]interface{}{
				"status": graphql.String("ALLOCATED"),
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxAllocatedHandler,
		},
		{
			Query: MandelboxStatusQuery,
			Variables: map[string]interface{}{
				"status": graphql.String("EXITED"),
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxAllocatedHandler,
		},
	}
	whistClient.SetSubscriptions(scalingSubscriptions)
}

// Start is the main function in the subscriptions package. It initializes a client, sets up the received subscriptions,
// and starts a goroutine for the client. It also has a goroutine to close the client and subscriptions when the global
// context gets cancelled.
func Start(whistClient WhistHasuraClient, globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan SubscriptionEvent) error {
	if !enabled {
		logger.Infof("Running in app environment %s so not enabling Hasura code.", metadata.GetAppEnvironment())
		return nil
	}

	// Slice to hold subscription IDs, necessary to properly unsubscribe when we are done.
	var subscriptionIDs []string

	// Initialize subscription client
	whistClient.Initialize()

	// Start goroutine that shuts down the client if the global context gets
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Listen for global context cancellation
		<-globalCtx.Done()
		whistClient.Close(whistClient.GetSubscriptionIDs())
	}()

	// Send subscriptions to the client
	for _, subscriptionParams := range whistClient.GetSubscriptions() {
		query := subscriptionParams.Query
		variables := subscriptionParams.Variables
		result := subscriptionParams.Result
		handler := subscriptionParams.Handler

		id, err := whistClient.Subscribe(query, variables, result, handler, subscriptionEvents)
		if err != nil {
			return err
		}
		subscriptionIDs = append(subscriptionIDs, id)
	}

	// Run the client
	whistClient.SetSubscriptionsIDs(subscriptionIDs)
	whistClient.Run(goroutineTracker)

	return nil
}
