package subscriptions

import (
	// 	"context"
	// 	"encoding/json"
	// 	"net/http"
	"context"
	"encoding/json"
	"net"
	"net/http"
	"testing"
	"time"

	// 	"time"

	"github.com/fractal/fractal/host-service/utils"
	graphql "github.com/hasura/go-graphql-client"
	"nhooyr.io/websocket"
	"nhooyr.io/websocket/wsjson"
)

func TestInstanceStatusHandler(t *testing.T) {
	// Start the mock server to communicate with the hasura client
	message := `{"data": {"cloud_instance_info": [{"instance_name": "test-instance-name","status": "DRAINING"}]}}`

	t.Log("Starting mock server and Hasura client...")
	port := mockServer(t, message)
	client := initClient(t, port)

	subscriptionEvents := make(chan SubscriptionEvent, 100)
	id, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

	t.Logf("Testing subscription with id: %v", id)
	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

	t.Log("Running Hasura subscriptions...")
	go client.Run()

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(*InstanceStatusEvent)

	gotInstance := instanceStatusEvent.InstanceInfo[0]

	// Verify that we got the expected values
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
	// Start the mock server to communicate with the hasura client
	message := `{"data": {"cloud_mandelbox_info": [{"instance_name": "test-instance-name","mandelbox_id":` +
		`"22222222-2222-2222-2222-222222222222","session_id": "1636666188732","user_id": "test-user-id","status": "ALLOCATED"}]}}`

	t.Log("Starting mock server and Hasura client...")
	port := mockServer(t, message)
	client := initClient(t, port)

	subscriptionEvents := make(chan SubscriptionEvent, 100)
	id, err := mandelboxInfoHandler("test-instance-name", "ALLOCATED", client, subscriptionEvents)

	t.Logf("Testing subscription with id: %v", id)
	if err != nil {
		t.Errorf("Error running instanceStatusHandler: %v", err)
	}

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

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}
}

func initClient(t *testing.T, port int) *graphql.SubscriptionClient {
	client := graphql.NewSubscriptionClient(utils.Sprintf("http://localhost:%v", port)).WithLog(t.Log).
		WithoutLogTypes(graphql.GQL_CONNECTION_KEEP_ALIVE).
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			t.Errorf("Error received from Hasura client: %v", err)
			return err
		})
	return client
}

func mockServer(t *testing.T, message string) int {
	mockserver := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		c, err := websocket.Accept(w, r, nil)

		if err != nil {
			t.Logf("Error accepting handshake from client: %v", err)
		}
		defer c.Close(websocket.StatusInternalError, "closing mock socket server")

		ctx, cancel := context.WithCancel(r.Context())
		defer cancel()

		time := time.NewTicker(time.Second)
		defer time.Stop()

		for {
			select {
			case <-ctx.Done():
				t.Log("Closing mock server due to context cancelled...")
				c.Close(websocket.StatusNormalClosure, "")
				return
			case <-time.C:
				var v interface{}
				err = wsjson.Read(ctx, c, &v)

				if err != nil {
					t.Log("received nil message on server")
					break
				}

				operation := v.(map[string]interface{})

				if operation["type"] == "start" {
					t.Log("Client successfully subscribed")
					jsonMessage := json.RawMessage(message)

					operationMessage := graphql.OperationMessage{
						ID:      operation["id"].(string),
						Type:    graphql.GQL_DATA,
						Payload: jsonMessage,
					}
					wsjson.Write(ctx, c, operationMessage)
					cancel()
				}
			}
		}
	})

	portchan := make(chan int)
	go func(portchan chan int) {
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
