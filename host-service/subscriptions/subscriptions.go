/*
Package subscriptions is responsible for implementing a pubsub architecture
on the host-service. This is achieved using Hasura live subscriptions, so that
the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import (
	"context"
	"encoding/json"
	"sync"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/metadata"
	graphql "github.com/hasura/go-graphql-client" // We use hasura's own graphql client for Go
)

// `enabled` is a flag denoting whether the functions in this package should do
// anything, or simply be no-ops. This is necessary, since we want the subscription
// operations to be meaningful in environments where we can expect the database
// guarantees to hold (e.g. `metadata.EnvLocalDevWithDB` or `metadata.EnvDev`)
// but no-ops in other environments.
var enabled = (metadata.GetAppEnvironment() != metadata.EnvLocalDev)

// StatusSubscriptionEvent is the event received from the subscription to any
// instance status changes.
type StatusSubscriptionEvent struct {
	InstanceInfo []Instance `json:"cloud_instance_info"`
	resultChan   chan InstanceInfoResult
}

// ReturnResult is called to pass the result of a subscription event back to the
// subbscription handler.
func (s *StatusSubscriptionEvent) ReturnResult(result interface{}, err error) {
	s.resultChan <- InstanceInfoResult{result}
}

// createResultChan is called to create the Go channel to pass the Hasura
// result back to the subscription handler.
func (s *StatusSubscriptionEvent) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan InstanceInfoResult)
	}
}

// StatusSubscriptionHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func StatusSubscriptionHandler(instanceName string, status string, client *graphql.SubscriptionClient, subscriptionEvents chan<- SubscriptionEvent) (string, error) {
	// variables holds the values needed to run the graphql subscription
	variables := map[string]interface{}{
		"instance_name": graphql.String(instanceName),
		"status":        graphql.String(status),
	}
	// This subscriptions fires when the running instance status changes to draining on the database
	id, err := client.Subscribe(SubscriptionInstanceStatus, variables, func(data *json.RawMessage, err error) error {

		if err != nil {
			return nil
		}

		var result StatusSubscriptionEvent
		json.Unmarshal(*data, &result)
		result.createResultChan()

		var instance Instance
		// If the result array returned by Hasura is not empty, it means
		// there was a change in the database row
		if len(result.InstanceInfo) > 0 {
			instance = result.InstanceInfo[0]
		} else {
			return nil
		}

		if (instance.InstanceName == instanceName) && (instance.Status == status) {
			// We notify via the subscriptionsEvent channel to start the drain and shutdown process
			subscriptionEvents <- &result
		}

		return nil
	})

	if err != nil {
		return "", err
	}

	return id, nil
}

// Run is responsible for starting the subscription client, as well as
// subscribing to the subscriptions defined in queries.go
func Run(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup, instanceName string, subscriptionEvents chan SubscriptionEvent) error {
	if !enabled {
		logger.Infof("Running in app environment %s so not enabling Hasura code.", metadata.GetAppEnvironment())
		return nil
	}
	// We set up the client with auth and logging parameters
	client, err := SetUpHasuraClient()

	if err != nil {
		// Error creating the hasura client
		return err
	}

	// Slice to hold subscription IDs, necessary to properly unsubscribe when we are done.
	var subscriptionIDs []string

	// Here we run all subscriptions
	id, err := StatusSubscriptionHandler(instanceName, "DRAINING", client, subscriptionEvents)
	if err != nil {
		// handle subscription error
		return err
	}
	subscriptionIDs = append(subscriptionIDs, id)

	// Start goroutine that shuts down hasura client if the global context gets
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Listen for global context cancellation
		<-globalCtx.Done()
		Close(client, subscriptionIDs)
	}()

	// Run the client on a go routine to make sure it closes properly when we are done
	go client.Run()

	return nil
}
