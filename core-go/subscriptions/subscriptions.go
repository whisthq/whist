/*
Package subscriptions is responsible for implementing a pubsub architecture
on the host-service. This is achieved using Hasura live subscriptions, so that
the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/fractal/core-go/subscriptions"

import (
	"context"
	"sync"

	"github.com/fractal/fractal/core-go/metadata"
	"github.com/fractal/fractal/core-go/metadata/aws"
	// We use hasura's own graphql client for Go
)

// `enabled` is a flag denoting whether the functions in this package should do
// anything, or simply be no-ops. This is necessary, since we want the subscription
// operations to be meaningful in environments where we can expect the database
// guarantees to hold (e.g. `metadata.EnvLocalDevWithDB` or `metadata.EnvDev`)
// but no-ops in other environments.
var enabled = (metadata.GetAppEnvironment() != metadata.EnvLocalDev)

// InstanceStatusEvent is the event received from the subscription to any
// instance status changes.
type InstanceEvent struct {
	InstanceInfo []Instance `json:"cloud_instance_info"`
}

// MandelboxInfoEvent is the event received from the subscription to any
// instance status changes.
type MandelboxEvent struct {
	MandelboxInfo []Mandelbox `json:"cloud_mandelbox_info"`
}

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

// MandelboxInfoHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxInfoHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
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

func SetupHostSubscriptions(instanceName aws.InstanceName, whistClient *WhistClient) {
	whistClient.Subscriptions = []HasuraSubscription{
		{
			Query: InstanceStatusSubscription,
			Variables: map[string]interface{}{
				"instanceName": instanceName,
				"status":       "DRAINING",
			},
			Result:  InstanceEvent{},
			Handler: InstanceStatusHandler,
		},
		{
			Query: MandelboxInfoSubscription,
			Variables: map[string]interface{}{
				"instanceName": instanceName,
				"status":       "ALLOCATED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxInfoHandler,
		},
	}
}

func SetupScalingSubscriptions(whistClient *WhistClient) {
	whistClient.Subscriptions = []HasuraSubscription{
		{
			Query: MandelboxStatusSubscription,
			Variables: map[string]interface{}{
				"status": "ALLOCATED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxInfoHandler,
		},
		{
			Query: MandelboxStatusSubscription,
			Variables: map[string]interface{}{
				"status": "EXITED",
			},
			Result:  MandelboxEvent{},
			Handler: MandelboxInfoHandler,
		},
	}
}

func Start(whistClient *WhistClient, globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan SubscriptionEvent) error {
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
		whistClient.Close(whistClient.SubscriptionIDs)
	}()

	for _, subscriptionParams := range whistClient.Subscriptions {
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

	whistClient.SubscriptionIDs = subscriptionIDs
	whistClient.Run(&goroutineTracker)

	return nil
}
