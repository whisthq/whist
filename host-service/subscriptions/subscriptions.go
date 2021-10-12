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
type InstanceStatusEvent struct {
	InstanceInfo []Instance `json:"cloud_instance_info"`
	resultChan   chan InstanceInfoResult
}

// ReturnResult is called to pass the result of a subscription event back to the
// subscription handler.
func (s *InstanceStatusEvent) ReturnResult(result interface{}, err error) {
	s.resultChan <- InstanceInfoResult{result}
}

// createResultChan is called to create the Go channel to pass the Hasura
// result back to the subscription handler.
func (s *InstanceStatusEvent) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan InstanceInfoResult)
	}
}

// instanceStatusHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func instanceStatusHandler(instanceName string, status string, client *graphql.SubscriptionClient, subscriptionEvents chan<- SubscriptionEvent) (string, error) {
	// variables holds the values needed to run the graphql subscription
	variables := map[string]interface{}{
		"instance_name": graphql.String(instanceName),
		"status":        graphql.String(status),
	}
	// This subscriptions fires when the running instance status changes to draining on the database
	id, err := client.Subscribe(InstanceStatusSubscription, variables, func(data *json.RawMessage, err error) error {

		if err != nil {
			return err
		}

		var result InstanceStatusEvent
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

// InstanceStatusEvent is the event received from the subscription to any
// instance status changes.
type MandelboxInfoEvent struct {
	MandelboxInfo []Mandelbox `json:"cloud_mandelbox_info"`
	resultChan    chan InstanceInfoResult
}

// ReturnResult is called to pass the result of a subscription event back to the
// subscription handler.
func (s *MandelboxInfoEvent) ReturnResult(result interface{}, err error) {
	s.resultChan <- InstanceInfoResult{result}
}

// createResultChan is called to create the Go channel to pass the Hasura
// result back to the subscription handler.
func (s *MandelboxInfoEvent) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan InstanceInfoResult)
	}
}

// mandelboxInfoHandler handles events from the hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func mandelboxInfoHandler(instanceName string, status string, client *graphql.SubscriptionClient, subscriptionEvents chan<- SubscriptionEvent) (string, error) {
	// variables holds the values needed to run the graphql subscription
	variables := map[string]interface{}{
		"instance_name": graphql.String(instanceName),
		"status":        graphql.String(status),
	}
	// This subscriptions fires when the running instance status changes to draining on the database
	id, err := client.Subscribe(MandelboxInfoSubscription, variables, func(data *json.RawMessage, err error) error {

		if err != nil {
			return nil
		}

		var result MandelboxInfoEvent
		json.Unmarshal(*data, &result)
		result.createResultChan()

		var mandelbox Mandelbox
		// If the result array returned by Hasura is not empty, it means
		// there was a change in the database row
		if len(result.MandelboxInfo) > 0 {
			mandelbox = result.MandelboxInfo[0]
		} else {
			return nil
		}

		if mandelbox.InstanceName == instanceName {
			// We notify via the subscriptionsEvent channel to spin up the mandelbox
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
	logger.Infof("Setting up Hasura subscriptions...")
	// We set up the client with auth and logging parameters
	client, err := SetUpHasuraClient()

	if err != nil {
		// Error creating the hasura client
		logger.Errorf("Error starting Hasura client: %v", err)
		return err
	}

	// Slice to hold subscription IDs, necessary to properly unsubscribe when we are done.
	var subscriptionIDs []string

	// Here we run all subscriptions
	id, err := instanceStatusHandler(instanceName, "DRAINING", client, subscriptionEvents)
	if err != nil {
		// handle subscription error
		return err
	}
	subscriptionIDs = append(subscriptionIDs, id)

	id, err = mandelboxInfoHandler(instanceName, "ALLOCATED", client, subscriptionEvents)
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
