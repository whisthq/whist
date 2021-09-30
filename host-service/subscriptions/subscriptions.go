/*
Package subscriptions is responsible for implementing a pubsub architecture
on the host-service. This is achieved using Hasura live subscriptions, so that
the host-service can be notified instantly of any change in the database
*/
package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import (
	"encoding/json"
	"log"

	graphql "github.com/hasura/go-graphql-client" // We use hasura's own graphql client for Go
)

// SetUpHasuraClient does all the necessary work to set up the Hasura client
func SetUpHasuraClient(url string, adminSecret string) *graphql.SubscriptionClient {
	client := graphql.NewSubscriptionClient(url).
		WithConnectionParams(map[string]interface{}{
			"headers": map[string]string{
				"x-hasura-admin-secret": adminSecret,
			},
		}).WithLog(log.Println).
		WithoutLogTypes(graphql.GQL_DATA, graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			log.Print("err", err)
			return err
		})
	return client
}

// Run is responsible for starting the subscription client, as well as
// subscribing to the subscriptions defined in queries.go
func Run(instanceName string, done chan bool) error {
	// We set up the client with auth and logging parameters
	url := "http://localhost:8080/v1/graphql"
	secret := "hasura"
	client := SetUpHasuraClient(url, secret)

	// Subscribe to the DRAINING status
	id, err := SubscribeStatus(instanceName, "DRAINING", client, done)
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

// SubscribeStatus handles the hasura subscription to detect changes on instance instanceName
// to the given status in the database.
func SubscribeStatus(instanceName string, status string, client *graphql.SubscriptionClient, done chan<- bool) (string, error) {
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

		var result SubscriptionStatusResult
		json.Unmarshal(*data, &result)

		var instance Instance
		// If the result array returned by Hasura is not empty, it means
		// there was a change in the database row
		if len(result.Hardware_instance_info) > 0 {
			instance = result.Hardware_instance_info[0]
		} else {
			return nil
		}

		if (instance.Instance_name == instanceName) && (instance.Status == status) {
			// We notify via the done channel to start the drain and shutdown process
			done <- true
		}

		return nil
	})

	if err != nil {
		return "", err
	}

	return id, nil
}
