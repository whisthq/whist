package subscriptions

import (
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/google/uuid"
	graphql "github.com/hasura/go-graphql-client"
)

// mockWhistClient is a struct that mocks a real Hasura client for testing.
type mockWhistClient struct {
	Params          HasuraParams
	Subscriptions   []HasuraSubscription
	SubscriptionIDs []string
}

func (cl *mockWhistClient) Initialize() error {
	return nil
}

func (cl *mockWhistClient) GetSubscriptions() []HasuraSubscription {
	return cl.Subscriptions
}

func (cl *mockWhistClient) SetSubscriptions(subscriptions []HasuraSubscription) {
	cl.Subscriptions = subscriptions
}

func (cl *mockWhistClient) GetSubscriptionIDs() []string {
	return cl.SubscriptionIDs
}

func (cl *mockWhistClient) SetSubscriptionsIDs(subscriptionIDs []string) {
	cl.SubscriptionIDs = subscriptionIDs
}

func (cl *mockWhistClient) GetParams() HasuraParams {
	return cl.Params
}

func (cl *mockWhistClient) SetParams(params HasuraParams) {
	cl.Params = params
}

// Subscribe mocks creating subscriptions. For testing, only returned predefined test events.
func (cl *mockWhistClient) Subscribe(query GraphQLQuery, variables map[string]interface{}, result SubscriptionEvent,
	conditionFn handlerfn, subscriptionEvents chan SubscriptionEvent) (string, error) {

	// Create fake instance event
	testInstanceEvent := InstanceEvent{InstanceInfo: []Instance{{
		InstanceName: variables["instanceName"].(string),
		Status:       variables["status"].(string),
	}}}

	// Create fake mandelbox event
	testMandelboxEvent := MandelboxEvent{MandelboxInfo: []Mandelbox{{
		InstanceName: variables["instanceName"].(string),
		Status:       "ALLOCATED",
	}}}

	// Send fake event through channel depending on the result type received
	switch result.(type) {

	case InstanceEvent:
		if conditionFn(testInstanceEvent, variables) {
			subscriptionEvents <- testInstanceEvent
		}
	case MandelboxEvent:
		if conditionFn(testMandelboxEvent, variables) {
			subscriptionEvents <- testMandelboxEvent
		}
	default:

	}

	return uuid.NewString(), nil
}

func (cl *mockWhistClient) Run(goroutinetracker *sync.WaitGroup) {}

func (cl *mockWhistClient) Close(subscriptionIDs []string) error {
	return nil
}
func TestInstanceStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"instance_name": graphql.String("test-instance-name"),
		"status":        graphql.String("DRAINING"),
	}

	// Start a mock socket server and Hasura client
	t.Log("Starting mock server and Hasura client...")
	port := mockServer(t, message)
	client := initClient(t, port)

	// Subscribe to the instance status event
	subscriptionEvents := make(chan SubscriptionEvent, 100)
	id, err := instanceStatusHandler("test-instance-name", "DRAINING", client, subscriptionEvents)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}
func TestMandelboxAllocatedHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"instance_name": graphql.String("test-instance-name"),
		"status":        graphql.String("ALLOCATED"),
	}

	// Run the Hasura client and subscription
	t.Log("Running Hasura subscriptions...")
	go client.Run()

	// Verify that the subscription result is sent to the appropiate channel
	subscriptionEvent := <-subscriptionEvents
	instanceStatusEvent := subscriptionEvent.(*InstanceEvent)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestMandelboxStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"status": graphql.String("ALLOCATED"),
	}

	testMap := []struct {
		key       string
		want, got string
	}{
		{"InstanceName", "test-instance-name", gotInstance.InstanceName},
		{"Status", "DRAINING", gotInstance.Status},
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"instance_name": graphql.String(instanceName),
		"status":        graphql.String("DRAINING"),
	}

	// Verify that the "variables" maps are deep equal for the first subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = graphql.String("ALLOCATED")

	// Verify that the "variables" maps are deep equal for the second subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
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

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"status": graphql.String("ALLOCATED"),
	}

	// Run the Hasura client and subscription
	t.Log("Running Hasura subscriptions...")
	go client.Run()

	if !reflect.DeepEqual(variables, whistClient.Subscriptions[0].Variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = graphql.String("EXITED")

	// Verify that the "variables" maps are deep equal for the second subscription
	if !reflect.DeepEqual(variables, whistClient.Subscriptions[1].Variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}

// TODO: Add test to subscriptions start function once scaling service is done.
