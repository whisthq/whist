package subscriptions

import (
	"context"
	"encoding/json"
	"net"
	"net/http"
	"testing"
	"time"

	"github.com/fractal/fractal/host-service/utils"
	graphql "github.com/hasura/go-graphql-client"
	"nhooyr.io/websocket"
	"nhooyr.io/websocket/wsjson"
)

func TestInstanceStatusHandler(t *testing.T) {
	// Create a mock Hasura data event as would be received from the database
	message := `{"data": {"cloud_instance_info": [{"instance_name": "test-instance-name","status": "DRAINING"}]}}`

	// Start a mock socket server and Hasura client
	t.Log("Starting mock server and Hasura client...")
	port := mockServer(t, message)
	client := initClient(t, port)

	// Subscribe to the instance status event
	subscriptionEvents := make(chan SubscriptionEvent, 100)
	id, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

	t.Logf("Testing subscription with id: %v", id)
	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	// Run the Hasura client and subscription
	t.Log("Running Hasura subscriptions...")
	go client.Run()

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(*InstanceStatusEvent)

	gotInstance := instanceStatusEvent.InstanceInfo[0]

	testMap := []struct {
		key       string
		want, got string
	}{
		{"InstanceName", "test-instance-name", gotInstance.InstanceName},
		{"Status", "DRAINING", gotInstance.Status},
	}

	// Verify that we got the expected values
	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}

}

func TestMandelboxInfoHandler(t *testing.T) {
	// Create a mock Hasura data event as would be received from the database
	message := `{"data": {"cloud_mandelbox_info": [{"instance_name": "test-instance-name","mandelbox_id":` +
		`"22222222-2222-2222-2222-222222222222","session_id": "1636666188732","user_id": "test-user-id","status": "ALLOCATED"}]}}`

	// Start a mock socket server and Hasura client
	t.Log("Starting mock server and Hasura client...")
	port := mockServer(t, message)
	client := initClient(t, port)

	// Subscribe to the mandelbox allocation event
	subscriptionEvents := make(chan SubscriptionEvent, 100)
	id, err := mandelboxInfoHandler("test-instance-name", "ALLOCATED", client, subscriptionEvents)

	t.Logf("Testing subscription with id: %v", id)
	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	// Run the Hasura client and subscription
	t.Log("Running Hasura subscriptions...")
	go client.Run()

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(*MandelboxInfoEvent)

	gotInstance := instanceStatusEvent.MandelboxInfo[0]

	testMap := []struct {
		key       string
		want, got string
	}{
		{"InstanceName", "test-instance-name", gotInstance.InstanceName},
		{"MandelboxID", utils.PlaceholderTestUUID().String(), gotInstance.ID.String()},
		{"SessionID", "1636666188732", string(gotInstance.SessionID)},
		{"UserID", "test-user-id", string(gotInstance.UserID)},
	}

	// Verify that we got the expected values
	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}
}

// initClient initializes a test Hasura client that behaves exactly like the one use on the production code
func initClient(t *testing.T, port int) *graphql.SubscriptionClient {
	client := graphql.NewSubscriptionClient(utils.Sprintf("http://localhost:%v", port)).WithLog(t.Log).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			t.Errorf("Error received from Hasura client: %v", err)
			return err
		})
	return client
}

// For testing the subscriptions, we need to create a mock socket server that
// is able to communicate with the Hasura client as if it were the Hasura server.
func mockServer(t *testing.T, message string) int {
	mockserver := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Accept handshake from client
		c, err := websocket.Accept(w, r, nil)
		if err != nil {
			t.Logf("Error accepting handshake from client: %v", err)
		}
		defer c.Close(websocket.StatusInternalError, "closing mock socket server")

		ctx, cancel := context.WithCancel(r.Context())
		defer cancel()

		// Create ticker to receive messages from the client
		time := time.NewTicker(time.Second)
		defer time.Stop()

		for {
			// Server loop
			select {
			case <-ctx.Done():
				t.Log("Closing mock server due to context cancelled...")
				c.Close(websocket.StatusNormalClosure, "")
				return
			case <-time.C:
				// Read message from client
				var v interface{}
				err = wsjson.Read(ctx, c, &v)

				if err != nil {
					t.Log("received nil message on server")
					break
				}

				operation := v.(map[string]interface{})
				// If the message matches the `start` operation
				// it means the client successfully started the
				// subscription.
				if operation["type"] == "start" {
					t.Log("Client successfully subscribed")
					jsonMessage := json.RawMessage(message)

					// Create a mock message to send to the client
					// We send the "database event" as a raw json message
					operationMessage := graphql.OperationMessage{
						ID:      operation["id"].(string),
						Type:    graphql.GQL_DATA,
						Payload: jsonMessage,
					}
					// Write the message once and cancel the context
					wsjson.Write(ctx, c, operationMessage)
					cancel()
				}
			}
		}
	})

	// Start the server on a go routine to avoid blocking
	// Use the portchan to return the port it is running on.
	portchan := make(chan int)
	go func(portchan chan int) {
		// We use random available ports to ensure the server always starts.,
		listener, err := net.Listen("tcp", ":0")
		if err != nil {
			panic(err)
		}

		port := listener.Addr().(*net.TCPAddr).Port
		t.Log("Using port:", port)
		portchan <- port
		t.Error(http.Serve(listener, mockserver))
	}(portchan)

	return <-portchan
}
