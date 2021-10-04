package subscriptions

import (
	"log"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	graphql "github.com/hasura/go-graphql-client"
)

// isRunning is a helper variable to close the connection to Hasura correctly,
// in case the global context gets canceled or a drain and shutdown event is received.
var isRunning = false

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
		}).WithLog(log.Println).
		WithoutLogTypes(graphql.GQL_DATA, graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			log.Print("err", err)
			return err
		})

	isRunning = true
	return client, nil
}

// Close manages all the logic to unsubscribe to every subscription and close the connection
// to the Hasura server correctly.
func Close(client *graphql.SubscriptionClient, subscriptionIDs []string, done chan<- bool) error {
	// We have to ensure we unsubscribe to every subscription
	// before closing the client, otherwise it will result in a deadlock!

	if isRunning {
		logger.Infof("Closing Hasura subscriptions...")
		for _, id := range subscriptionIDs {
			// This is safe to do because the Unsubscribe method
			// acquires a lock when closing the connection.
			err := client.Unsubscribe(id)

			if err != nil {
				logger.Errorf("Failed to unsubscribe from:%v", id)
				return err
			}
		}

		// Once we have successfully unsubscribed, close the connection to the
		// Hasura server.
		logger.Infof("Closing connection to Hasura server...")
		err := client.Close()
		if err != nil {
			logger.Errorf("Error closing connection with Hasura server.")
			return err
		}
		isRunning = false
	} else {
		logger.Infof("Hasura client already closed.")
	}

	done <- true // Notify the client closed successfully.

	return nil
}
