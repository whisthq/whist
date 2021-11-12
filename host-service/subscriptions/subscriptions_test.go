package subscriptions

import (
	"context"
	"encoding/json"
	"net/http"
	"testing"
	"time"

	graphql "github.com/hasura/go-graphql-client"
	"nhooyr.io/websocket"
	"nhooyr.io/websocket/wsjson"
)

func TestInstanceStatusHandler(t *testing.T) {
	message := `{"cloud_instance_info": [{"instance_name": "test-instance-name","status": "DRAINING"}]}`

	// Start the mock server to communicate with the hasura client
	mockServer(t, message)

	t.Log("Starting Hasura client...")
	client := graphql.NewSubscriptionClient("http://localhost:8082").WithLog(t.Log).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			t.Errorf("Error received from Hasura client: %v", err)
			return err
		})

	t.Log("Subscribing to instance status event...")
	subscriptionEvents := make(chan SubscriptionEvent, 100)

	id, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

	t.Logf("Testing subscription with id: %v", id)
	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	t.Log("Starting Hasura client...")
	go client.Run()
	t.Log("Verifying subscription result channel...")
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
	message := `{"data": {"cloud_mandelbox_info": [{"instance_name": "test-instance-id","mandelbox_id":` +
		`22222222-2222-2222-2222-222222222222","session_id": "1636666188732","user_id": "test-user-id","status": "RUNNING"},]}}`

	// Start the mock server to communicate with the hasura client
	mockServer(t, message)
	client := graphql.NewSubscriptionClient("http://localhost:8082")

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
		{"MandelboxID", "22222222-2222-2222-2222-222222222222", gotInstance.ID.String()},
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

		var v interface{}
		err = wsjson.Read(ctx, c, &v)
		if err != nil {
			t.Errorf("Error reading json message on server")
		}

		t.Logf("received: %v", v)

		operationType := v.(map[string]interface{})["type"]

		if operationType == "start" {
			t.Log("Client successfully subscribed")

			jsonMessage := json.RawMessage(message)

			operation := graphql.OperationMessage{
				ID:      "test-operation",
				Type:    graphql.GQL_DATA,
				Payload: jsonMessage,
			}

			t.Log("Sending message to client...")
			wsjson.Write(ctx, c, operation)
			// c.Close(websocket.StatusNormalClosure, "")
		}
	})

	t.Log("Starting mock socket server...")
	go func() {
		err := http.ListenAndServe("localhost:8082", mockserver)

		if err != nil {
			t.Errorf("Error starting mock server: %v", err)
		}
	}()

}
