package subscriptions

import (
	"log"

	graphql "github.com/hasura/go-graphql-client"
)

// SetUpHasuraClient does all the necessary work to set up the Hasura client
func SetUpHasuraClient() (*graphql.SubscriptionClient, error) {
	p, err := getFractalHasuraParams()
	params := p.(HasuraParams)

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
	return client, nil
}

func Close(client *graphql.SubscriptionClient, subscriptionIDs []string, done chan<- bool) error {
	// We have to ensure we unsubscribe to every subscription
	// before closing the client, otherwise it will result in a deadlock!

	for _, id := range subscriptionIDs {
		// This is safe to do because the Unsubscribe method
		// acquires a lock when closing the connection.
		err := client.Unsubscribe(id)

		if err != nil {
			return err
		}
	}

	// Once we have successfully unsubscribed, close the connection to the
	// Hasura server.
	client.Close()
	done <- true // Notify the client closed successfully.
	return nil
}
