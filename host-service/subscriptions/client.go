package subscriptions

import (
	logger "github.com/fractal/fractal/host-service/whistlogger"
	graphql "github.com/hasura/go-graphql-client"
)

// SetUpHasuraClient does all the necessary work to set up the Hasura client
func SetUpHasuraClient() (*graphql.SubscriptionClient, error) {
	params, err := getFractalHasuraParams()

	if err != nil {
		// Error obtaining the connection parameters, we stop and don't setup the client
		return nil, err
	}

	client := graphql.NewSubscriptionClient(params.URL).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"x-hasura-admin-secret": params.AccessKey,
			},
		}).WithLog(logger.Print).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			logger.Errorf("Error received from Hasura client: %v", err)
			return err
		})

	return client, nil
}

// Close manages all the logic to unsubscribe to every subscription and close the connection
// to the Hasura server correctly.
func Close(client *graphql.SubscriptionClient, subscriptionIDs []string) error {
	// We have to ensure we unsubscribe to every subscription
	// before closing the client, otherwise it will result in a deadlock!
	logger.Infof("Closing Hasura subscriptions...")
	for _, id := range subscriptionIDs {
		// This is safe to do because the Unsubscribe method
		// acquires a lock when closing the connection.
		err := client.Unsubscribe(id)

		if err != nil {
			logger.Errorf("Failed to unsubscribe from:%v, %v", id, err)
			return err
		}
	}

	// Once we have successfully unsubscribed, close the connection to the
	// Hasura server.
	logger.Infof("Closing connection to Hasura server...")
	err := client.Close()
	if err != nil {
		// Only use a warning instead of an error because failure to close the
		// Hasura server is not fatal, as we have already started the host service
		// shut down process and the client will get cleaned up.
		logger.Warningf("Error closing connection with Hasura server: %v", err)
		return err
	}

	return nil
}
