/*
	The subscriptions package is responsible for implementing a pubsub architecture
	on the host-service. This is achieved using Hasura live subscriptions, so that
	the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import (
	"encoding/json"
	"log"

	graphql "github.com/hasura/go-graphql-client" // We use hasura's own graphql client for Go
)

// Run is responsible for starting the subscription client, as well as
// subscribing to the subscriptions defined in queries.go
func Run(instanceName string, done chan bool) error {
	// We set up the client with auth and logging parameters
	url := "http://localhost:8080/v1/graphql"
	client := graphql.NewSubscriptionClient(url).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"x-hasura-admin-secret": "hasura",
			},
		}).WithLog(log.Println).
		WithoutLogTypes(graphql.GQL_DATA, graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			log.Print("err", err)
			return err
		})

	// variables holds the values needed to run the graphql subscription
	variables := map[string]interface{}{
		"instance_name": graphql.String(instanceName),
		"status":        graphql.String("DRAINING"),
	}

	// This subscriptions fires when the running instance status changes to draining on the database
	id, err := client.Subscribe(SubscriptionInstanceStatus, variables, func(data *json.RawMessage, err error) error {

		if err != nil {
			return nil
		}
		// We notify via the done channel to start the drain and shutdown process
		done <- true
		return nil
	})

	if err != nil {
		// handle subscription error
		return err
	}

	defer func(id string) {
		// We have to ensure we unsubscribe before closing the client,
		// otherwise it will result in a deadlock!
		client.Unsubscribe(id)
		client.Close()
		done <- true
	}(id)

	// Run the client on a go routine to make sure it closes properly when we are done
	go client.Run()

	<-done
	return nil
}
