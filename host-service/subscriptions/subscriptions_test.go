package subscriptions

import (
	"context"
	"net/http"
	"testing"
	"time"

	graphql "github.com/hasura/go-graphql-client"
	"nhooyr.io/websocket"
	"nhooyr.io/websocket/wsjson"
)

func TestInstanceStatusHandler(t *testing.T) {
	message := `{
		"data": {
		  "cloud_instance_info": [
			{
			  "instance_name": "test-instance-name",
			  "status": "DRAINING"
			},
		  ]
		}
	  }`

	// Start the mock server to communicate with the hasura client
	mockServer(t, message)
	client := graphql.NewSubscriptionClient("localhost:8080/graphql")

	subscriptionEvents := make(chan SubscriptionEvent, 100)
	_, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(InstanceStatusEvent)

	gotInstance := instanceStatusEvent.InstanceInfo[0]

	testMap := []struct {
		key       string
		want, got string
	}{
		{"InstanceName", "test-instance-name", gotInstance.InstanceName},
		{"Status", "DRAINING", gotInstance.Status},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}
}

func TestMandelboxInfoHandler(t *testing.T) {
	message := `{
		"data": {
		  "cloud_mandelbox_info": [
			{
			  "instance_name": "test-instance-id",
			  "mandelbox_id": "22222222-2222-2222-2222-222222222222",
			  "session_id": "1636666188732",
			  "user_id": "test-user-id",
			  "status": "RUNNING"
			},
		  ]
		}
	  }`

	// Start the mock server to communicate with the hasura client
	mockServer(t, message)
	client := graphql.NewSubscriptionClient("localhost:8080/graphql")

	subscriptionEvents := make(chan SubscriptionEvent, 100)
	_, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(MandelboxInfoEvent)

	gotInstance := instanceStatusEvent.MandelboxInfo[0]

	// InstanceName string                     `json:"instance_name"`
	// ID           mandelboxtypes.MandelboxID `json:"mandelbox_id"`
	// SessionID    string                     `json:"session_id"`
	// UserID       mandelboxtypes.UserID      `json:"user_id"`

	testMap := []struct {
		key       string
		want, got string
	}{
		{"InstanceName", "test-instance-name", gotInstance.InstanceName},
		{"MandelboxID", "22222222-2222-2222-2222-222222222222", string(gotInstance.ID)},
		{"SessionID", "1636666188732", string(gotInstance.UserID)},
		{"UserID", "test-user-id", string(gotInstance.SessionID)},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}
}

func mockServer(t *testing.T, message string) {
	mockserver := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		c, err := websocket.Accept(w, r, nil)

		if err != nil {
			t.Logf("Error accepting handshake from client: %v", err)
		}
		defer c.Close(websocket.StatusInternalError, "closing mock socket server")

		ctx, cancel := context.WithTimeout(r.Context(), time.Second*10)
		defer cancel()

		// var v interface{}
		// err = wsjson.Read(ctx, c, &v)
		// if err != nil {
		// 	t.Errorf("Error reading json message on server")
		// }

		// t.Logf("received: %v", v)
		wsjson.Write(ctx, c, message)
		c.Close(websocket.StatusNormalClosure, "")
	})

	err := http.ListenAndServe("localhost:8080/graphql", mockserver)

	if err != nil {
		t.Errorf("Error starting mock server: %v", err)
	}
}
