package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import (
	"encoding/json"
	"log"
	"time"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	graphql "github.com/hasura/go-graphql-client"
)

func Run() (<-chan SubscriptionStatusResult, error) {
	events := make(chan SubscriptionStatusResult, 100)

	url := "http://localhost:8080/v1/graphql"
	Client := graphql.NewSubscriptionClient(url).
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

	defer Client.Close()

	variables := map[string]interface{}{
		// "instance_name": graphql.String(""),
		"status": graphql.String("DRAINING"),
	}

	// This subscriptions fires when the instance status is draining
	_, err := Client.Subscribe(&SubscriptionInstanceStatus, variables, func(message *json.RawMessage, err error) error {
		if err != nil {
			logger.Errorf("Subscription error: %v", err)
			return nil
		}

		var result SubscriptionStatusResult

		json.Unmarshal(*message, &result)
		logger.Infof("DB Event: %v", result)
		if result.Hardware_instance_info[0].Status == "DRAINING" {
			events <- result
		}
		return nil
	})

	if err != nil {
		// handle subscription error
		logger.Errorf("Subscription error!")
	}

	go Client.Run()
	time.Sleep(time.Minute)

	return events, nil
}
