/*
Package subscriptions is responsible for implementing a pubsub architecture
on the Whist backend services. This is achieved using Hasura live subscriptions, so that
the host-service  and scaling-service can be notified instantly of any change in the database.
*/
package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"context"
	"sync"

	graphql "github.com/hasura/go-graphql-client" // We use Hasura's own GraphQL client for Go
	"github.com/whisthq/whist/backend/services/metadata"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// InstanceStatusHandler handles events from the Hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func InstanceStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(InstanceEvent)

	var instance Instance
	if len(result.Instances) > 0 {
		instance = result.Instances[0]
	}

	if variables["id"] == nil {
		return false
	}

	id := string(variables["id"].(graphql.String))

	return (instance.ID == id)
}

// InstancesStatusHandler handles events from the Hasura subscription which
// detects changes on all instances to the given status in the database.
func InstancesStatusHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(InstanceEvent)

	var instance Instance
	if len(result.Instances) > 0 {
		instance = result.Instances[0]
	}

	if variables["status"] == nil {
		return false
	}

	status := string(variables["status"].(graphql.String))

	return instance.Status == status
}

// MandelboxAllocatedHandler handles events from the Hasura subscription which
// detects changes on instance instanceName to the given status in the database.
func MandelboxAllocatedHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(MandelboxEvent)

	var mandelbox Mandelbox
	if len(result.Mandelboxes) > 0 {
		mandelbox = result.Mandelboxes[0]
	}

	if variables["instance_id"] == nil {
		return false
	}

	instanceID := string(variables["instance_id"].(graphql.String))
	status := string(variables["status"].(graphql.String))

	return (mandelbox.InstanceID == instanceID) && (mandelbox.Status == status)
}

// ClientAppVersionHandler handles events from the hasura subscription which
// detects changes on the config database client app version to the current.
func ClientAppVersionHandler(event SubscriptionEvent, variables map[string]interface{}) bool {
	result := event.(ClientAppVersionEvent)

	var (
		version    ClientAppVersion
		commitHash string
	)

	if len(result.ClientAppVersions) > 0 {
		version = result.ClientAppVersions[0]
	}

	if version == (ClientAppVersion{}) {
		return false
	}

	// get the commit hash of the environment we are running in
	switch metadata.GetAppEnvironmentLowercase() {
	case string(metadata.EnvDev):
		commitHash = version.DevCommitHash
	case string(metadata.EnvStaging):
		commitHash = version.StagingCommitHash
	case string(metadata.EnvProd):
		commitHash = version.ProdCommitHash
	default:
		commitHash = version.DevCommitHash
	}

	return metadata.GetGitCommit() == commitHash
}

// SetupHostSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the host-service.
func SetupHostSubscriptions(instanceID string, whistClient WhistSubscriptionClient) {
	hostSubscriptions := []HasuraSubscription{
		{
			Query: QueryInstanceById,
			Variables: map[string]interface{}{
				"id": graphql.String(instanceID),
			},
			Result:  InstanceEvent{[]Instance{}},
			Handler: InstanceStatusHandler,
		},
		{
			Query: QueryMandelboxesByInstanceId,
			Variables: map[string]interface{}{
				"instance_id": graphql.String(instanceID),
				"status":      graphql.String("ALLOCATED"),
			},
			Result:  MandelboxEvent{[]Mandelbox{}},
			Handler: MandelboxAllocatedHandler,
		},
	}
	whistClient.SetSubscriptions(hostSubscriptions)
}

// SetupScalingSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used on the scaling-service.
func SetupScalingSubscriptions(whistClient WhistSubscriptionClient) {
	scalingSubscriptions := []HasuraSubscription{
		{
			Query: QueryInstancesByStatus,
			Variables: map[string]interface{}{
				"status": graphql.String("DRAINING"),
			},
			Result:  InstanceEvent{[]Instance{}},
			Handler: InstancesStatusHandler,
		},
	}
	whistClient.SetSubscriptions(scalingSubscriptions)
}

// SetupConfigSubscriptions creates a slice of HasuraSubscriptions to start the client. This
// function is specific for the subscriptions used for the config database.
func SetupConfigSubscriptions(whistClient WhistSubscriptionClient) {
	// The version ID is always set to 1 on the database, since there
	// is only one row in the `desktop_client_app_version` which contains
	// all of the dev/staging/prod commit hashes.
	const versionID = 1

	configSubscriptions := []HasuraSubscription{
		{
			Query: QueryClientAppVersion,
			Variables: map[string]interface{}{
				"id": graphql.Int(versionID),
			},
			Result:  ClientAppVersionEvent{ClientAppVersions: []ClientAppVersion{}},
			Handler: ClientAppVersionHandler,
		},
	}
	whistClient.SetSubscriptions(configSubscriptions)
}

// Start is the main function in the subscriptions package. It initializes a client, sets up the received subscriptions,
// and starts a goroutine for the client. It also has a goroutine to close the client and subscriptions when the global
// context gets cancelled. It's possible to subscribe to the config database instead of the dev/staging/prod database by
// setting the `useConfigDatabase` argument.
func Start(whistClient WhistSubscriptionClient, globalCtx context.Context, goroutineTracker *sync.WaitGroup, subscriptionEvents chan SubscriptionEvent, useConfigDB bool) error {
	if !Enabled {
		logger.Infof("Running in app environment %s so not enabling Subscription client code.", metadata.GetAppEnvironment())
		return nil
	}

	// Slice to hold subscription IDs, necessary to properly unsubscribe when we are done.
	var subscriptionIDs []string

	// Initialize subscription client
	whistClient.Initialize(useConfigDB)

	// Start goroutine that shuts down the client if the global context gets
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Listen for global context cancellation
		<-globalCtx.Done()
		whistClient.Close()
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
