package subscriptions

import (
	"encoding/json"
	"sync"

	"github.com/fractal/fractal/core-go/metadata"
	"github.com/fractal/fractal/core-go/utils"
	logger "github.com/fractal/fractal/core-go/whistlogger"
	graphql "github.com/hasura/go-graphql-client"
)

type WhistHasuraClient interface {
	Initialize()
	Subscribe()
	Run()
	Close()
}

type WhistClient struct {
	Hasura          *graphql.SubscriptionClient
	Params          HasuraParams
	Subscriptions   []HasuraSubscription
	SubscriptionIDs []string
}

func (whistClient *WhistClient) Initialize() {
	whistClient.Hasura = graphql.NewSubscriptionClient(whistClient.Params.URL).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"x-hasura-admin-secret": whistClient.Params.AccessKey,
			},
		}).WithLog(logger.Print).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			logger.Errorf("Error received from Hasura client: %v", err)
			return err
		})
}

func (whistClient *WhistClient) Subscribe(query GraphQLQuery, variables map[string]interface{}, result SubscriptionEvent, conditionFn handlerfn, subscriptionEvents chan SubscriptionEvent) (string, error) {
	// This subscriptions fires when the running instance status changes to draining on the database
	id, err := whistClient.Hasura.Subscribe(query, variables, func(data *json.RawMessage, err error) error {
		if err != nil {
			return utils.MakeError("error receiving subscription event from Hasura: %v", err)
		}

		err = json.Unmarshal(*data, &result)
		if err != nil {
			return utils.MakeError("failed to unmarshal subscription event: %v", err)
		}

		if conditionFn(result, variables) {
			// We notify via the subscriptionsEvent channel
			subscriptionEvents <- &result
		}
		return nil
	})

	if err != nil {
		return "", err
	}

	return id, nil
}

// Run is responsible for starting the subscription client.
func (whistClient *WhistClient) Run(goroutineTracker *sync.WaitGroup) {
	// Run the client on a goroutine to make sure it closes properly when we are done
	goroutineTracker.Add(1)
	go func() {
		whistClient.Hasura.Run()
		goroutineTracker.Done()
	}()
}

// Close manages all the logic to unsubscribe to every subscription and close the connection
// to the Hasura server correctly.
func (whistClient *WhistClient) Close(subscriptionIDs []string) error {
	// We have to ensure we unsubscribe to every subscription
	// before closing the client, otherwise it will result in a deadlock!
	logger.Infof("Closing Hasura subscriptions...")
	for _, id := range subscriptionIDs {
		// This is safe to do because the Unsubscribe method
		// acquires a lock when closing the connection.
		err := whistClient.Hasura.Unsubscribe(id)

		if err != nil {
			logger.Errorf("Failed to unsubscribe from:%v, %v", id, err)
			return err
		}
	}

	// Once we have successfully unsubscribed, close the connection to the
	// Hasura server.
	logger.Infof("Closing connection to Hasura server...")
	err := whistClient.Hasura.Close()
	if err != nil {
		// Only use a warning instead of an error because failure to close the
		// Hasura server is not fatal, as we have already started the host service
		// shut down process and the client will get cleaned up.
		logger.Warningf("Error closing connection with Hasura server: %v", err)
		return err
	}

	return nil
}

func InitializeWhistHasuraClient(whistClient *WhistClient) error {
	if !enabled {
		logger.Infof("Running in app environment %s so not enabling Hasura code.", metadata.GetAppEnvironment())
		return nil
	}
	logger.Infof("Setting up Hasura subscriptions...")

	params, err := getWhistHasuraParams()
	if err != nil {
		// Error obtaining the connection parameters, we stop and don't setup the client
		return utils.MakeError("error creating hasura client: %v", err)
	}

	whistClient.Params = params
	whistClient.Initialize()

	return err
}
