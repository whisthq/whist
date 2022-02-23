package subscriptions

import (
	"encoding/json"
	"sync"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"

	graphql "github.com/hasura/go-graphql-client"
)

// `enabled` is a flag denoting whether the functions in this package should do
// anything, or simply be no-ops. This is necessary, since we want the subscription
// operations to be meaningful in environments where we can expect the database
// guarantees to hold (e.g. `metadata.EnvLocalDevWithDB` or `metadata.EnvDev`)
// but no-ops in other environments.
var enabled = (metadata.GetAppEnvironment() != metadata.EnvLocalDev)

// WhistSubscriptionClient is an interface used to abstract the interactions with
// the official Hasura client.
type WhistSubscriptionClient interface {
	Initialize() error
	GetSubscriptions() []HasuraSubscription
	SetSubscriptions([]HasuraSubscription)
	GetSubscriptionIDs() []string
	SetSubscriptionsIDs([]string)
	GetParams() HasuraParams
	SetParams(HasuraParams)
	Subscribe(GraphQLQuery, map[string]interface{}, SubscriptionEvent, Handlerfn, chan SubscriptionEvent) (string, error)
	Run(*sync.WaitGroup)
	Close([]string) error
}

// SubscriptionClient implements WhistSubscriptionClient and is exposed to be used
// by any other service that need to interact with the Hasura client.
type SubscriptionClient struct {
	Hasura          *graphql.SubscriptionClient
	Params          HasuraParams
	Subscriptions   []HasuraSubscription
	SubscriptionIDs []string
}

// Initialize creates the client. This function is respinsible from fetching the server
// information from Heroku.
func (wc *SubscriptionClient) Initialize() error {
	logger.Infof("Setting up Subscription client...")

	params, err := getWhistHasuraParams()
	if err != nil {
		// Error obtaining the connection parameters, we stop and don't setup the client
		return utils.MakeError("error creating hasura client: %v", err)
	}

	wc.SetParams(params)

	sc := graphql.NewSubscriptionClient(wc.GetParams().URL).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"x-hasura-admin-secret": wc.GetParams().AccessKey,
			},
		}).WithLog(logger.Print).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			logger.Errorf("Error received from Hasura client: %v", err)
			return err
		})

	// Use our custom websocket connection
	sc.WithWebSocket(WhistWebsocketConn)
	wc.Hasura = sc

	return nil
}

func (wc *SubscriptionClient) GetSubscriptions() []HasuraSubscription {
	return wc.Subscriptions
}

func (wc *SubscriptionClient) SetSubscriptions(subscriptions []HasuraSubscription) {
	wc.Subscriptions = subscriptions
}

func (wc *SubscriptionClient) GetSubscriptionIDs() []string {
	return wc.SubscriptionIDs
}

func (wc *SubscriptionClient) SetSubscriptionsIDs(ids []string) {
	wc.SubscriptionIDs = ids
}

func (wc *SubscriptionClient) GetParams() HasuraParams {
	return wc.Params
}

func (wc *SubscriptionClient) SetParams(params HasuraParams) {
	wc.Params = params
}

// Subscribe creates the subscriptions according to the received queries and conditions.
// It passes results through the received channel if the received `conditionFn` is true.
func (wc *SubscriptionClient) Subscribe(query GraphQLQuery, variables map[string]interface{}, result SubscriptionEvent,
	conditionFn Handlerfn, subscriptionEvents chan SubscriptionEvent) (string, error) {

	id, err := wc.Hasura.Subscribe(query, variables, func(data *json.RawMessage, err error) error {
		if err != nil {
			return utils.MakeError("error receiving subscription event from Hasura: %v", err)
		}

		// Note: this switch is necessary to unmarshal the result into the appropiate
		// event type. Otherwise it will be unmarshalled as a map[string]interface{}
		switch result := result.(type) {
		case InstanceEvent:
			err = json.Unmarshal(*data, &result)

			if err != nil {
				return utils.MakeError("failed to unmarshal subscription event: %v", err)
			}

			if conditionFn(result, variables) {
				// We notify via the subscriptionsEvent channel
				subscriptionEvents <- &result
			}

		case MandelboxEvent:
			err = json.Unmarshal(*data, &result)

			if err != nil {
				return utils.MakeError("failed to unmarshal subscription event: %v", err)
			}

			if conditionFn(result, variables) {
				// We notify via the subscriptionsEvent channel
				subscriptionEvents <- &result
			}
		}

		return nil
	})

	if err != nil {
		return "", err
	}

	return id, nil
}

// Run is responsible for starting the subscription client and adding it
// to the routine tracker.
func (wc *SubscriptionClient) Run(goroutineTracker *sync.WaitGroup) {
	// Run the client on a goroutine to make sure it closes properly when we are done
	goroutineTracker.Add(1)
	go func() {
		wc.Hasura.Run()
		goroutineTracker.Done()
	}()
}

// Close manages all the logic to unsubscribe to every subscription and close the connection
// to the Hasura server correctly.
func (wc *SubscriptionClient) Close(subscriptionIDs []string) error {
	// We have to ensure we unsubscribe to every subscription
	// before closing the client, otherwise it will result in a deadlock!
	logger.Infof("Closing Hasura subscriptions...")
	for _, id := range subscriptionIDs {
		// This is safe to do because the Unsubscribe method
		// acquires a lock when closing the connection.
		err := wc.Hasura.Unsubscribe(id)

		if err != nil {
			// Only use a warning instead of an error because failure to unsubscribe
			// is not fatal, as we have already started the host service
			// shut down process and the client will get cleaned up.
			logger.Warningf("Failed to unsubscribe from:%v, %v", id, err)
		}
	}

	var err error
	// Once we have successfully unsubscribed, close the connection to the
	// Hasura server. Only do so if `wc.Hasura` is nil, otherwise the close
	// function will attempt to do so with a nil pointer.
	logger.Infof("Closing connection to Hasura server...")
	if wc.Hasura != nil {
		err = wc.Hasura.Close()
	}

	if err != nil {
		// Only use a warning instead of an error because failure to close the
		// Hasura server is not fatal, as we have already started the host service
		// shut down process and the client will get cleaned up.
		logger.Warningf("Error closing connection with Hasura server: %v", err)
	}

	// Once successfully close, set the Hasura field to nil
	wc.Hasura = nil
	return nil
}
