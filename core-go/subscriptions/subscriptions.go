/*
Package subscriptions is responsible for implementing a pubsub architecture
on the host-service. This is achieved using Hasura live subscriptions, so that
the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/fractal/core-go/subscriptions"

import (
	"context"
	"sync"

	"github.com/fractal/fractal/core-go/metadata/aws"
	// We use hasura's own graphql client for Go
)

// InstanceStatusHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func InstanceStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(InstanceEvent)

	var instance Instance
	if len(result.InstanceInfo) > 0 {
		instance = result.InstanceInfo[0]
	}

	return (instance.InstanceName == variables["instanceName"]) && (instance.Status == variables["status"])
}

// MandelboxAllocatedHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxAllocatedHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(MandelboxEvent)

	var mandelbox Mandelbox
	if len(result.MandelboxInfo) > 0 {
		mandelbox = result.MandelboxInfo[0]
	}

	return mandelbox.InstanceName == variables["instanceName"]
}

// MandelboxStatusHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(MandelboxEvent)

	var mandelbox Mandelbox
	if len(result.MandelboxInfo) > 0 {
		mandelbox = result.MandelboxInfo[0]
	}

	return mandelbox.Status == variables["status"]
}

// SetupHostSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the host service.
func SetupHostSubscriptions(instanceName aws.InstanceName, whistClient WhistHasuraClient) {
	client := whistClient.(*WhistClient)
	client.Subscriptions = []HasuraSubscription{
		{
			Query: InstanceStatusQuery,
			Variables: map[string]interface{}{
				"instanceName": instanceName,
				"status":       "DRAINING",
			},
			Result:  InstanceEvent{},
			Handler: InstanceStatusHandler,
		},
		{
			Query: MandelboxAllocatedQuery,
			Variables: map[string]interface{}{
				"instanceName": instanceName,
				"status":       "ALLOCATED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxAllocatedHandler,
		},
	}
}

// SetupScalingSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the scaling service.
func SetupScalingSubscriptions(whistClient WhistHasuraClient) {
	client := whistClient.(*WhistClient)
	client.Subscriptions = []HasuraSubscription{
		{
			Query: MandelboxStatusQuery,
			Variables: map[string]interface{}{
				"status": "ALLOCATED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxAllocatedHandler,
		},
		{
			Query: MandelboxStatusQuery,
			Variables: map[string]interface{}{
				"status": "EXITED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxAllocatedHandler,
		},
	}
}

// Start is the main function in the subscriptions package. It initializes a client, sets up the received subscriptions,
// and starts a goroutine for the client. It also has a goroutine to close the client and subscriptions when the global
// context gets cancelled.
func Start(whistClient WhistHasuraClient, globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan SubscriptionEvent) error {
	// Slice to hold subscription IDs, necessary to properly unsubscribe when we are done.
	var subscriptionIDs []string

	// Initialize subscription client
	client := whistClient.(*WhistClient)
	client.Initialize()

	// Start goroutine that shuts down the client if the global context gets
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Listen for global context cancellation
		<-globalCtx.Done()
		whistClient.Close(client.SubscriptionIDs)
	}()

	// Send subscriptions to the client
	for _, subscriptionParams := range client.Subscriptions {
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
	client.SubscriptionIDs = subscriptionIDs
	client.Run(goroutineTracker)

	return nil
}
