package subscriptions

import (
	"reflect"
	"sync"
	"testing"

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
	testInstanceEvent := InstanceEvent{Instances: []Instance{{
		ID:     variables["id"].(string),
		Status: variables["status"].(string),
	}}}

	// Create fake mandelbox event
	testMandelboxEvent := MandelboxEvent{Mandelboxes: []Mandelbox{{
		InstanceID: variables["instance_id"].(string),
		Status:     "ALLOCATED",
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
		"id":     graphql.String("test-instance-id"),
		"status": graphql.String("DRAINING"),
	}

	// Create different tests for the instance status handler,
	// verify if it returns the appropiate response
	var instanceTests = []struct {
		testName string
		event    InstanceEvent
		want     bool
	}{
		{"Empty event", InstanceEvent{Instances: []Instance{}}, false},
		{"Wrong status event", InstanceEvent{
			Instances: []Instance{
				{ID: "test-instance-id", Status: "PRE_CONNECTION"},
			},
		}, false},
		{"Correct status event", InstanceEvent{
			Instances: []Instance{
				{ID: "test-instance-id", Status: "DRAINING"},
			},
		}, true},
	}

	for _, tt := range instanceTests {
		testname := tt.testName
		t.Run(testname, func(t *testing.T) {
			got := InstanceStatusHandler(tt.event, variables)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}
func TestMandelboxAllocatedHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"instance_id": graphql.String("test-instance-id"),
		"status":      graphql.String("ALLOCATED"),
	}

	// Create different tests for the mandelbox allocated handler,
	// verify if it returns the appropiate response
	var mandelboxTests = []struct {
		testName string
		event    MandelboxEvent
		want     bool
	}{
		{"Empty event", MandelboxEvent{Mandelboxes: []Mandelbox{}}, false},
		{"Wrong instance id event", MandelboxEvent{
			Mandelboxes: []Mandelbox{
				{InstanceID: "test-instance-id-2", Status: "EXITED"},
			},
		}, false},
		{"Correct status event", MandelboxEvent{
			Mandelboxes: []Mandelbox{
				{InstanceID: "test-instance-id", Status: "ALLOCATED"},
			},
		}, true},
	}

	for _, tt := range mandelboxTests {
		testname := tt.testName
		t.Run(testname, func(t *testing.T) {
			got := MandelboxAllocatedHandler(tt.event, variables)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestSetupHostSubscriptions(t *testing.T) {
	instanceID := "test-instance-id"
	whistClient := &mockWhistClient{}

	// Create the host service specific subscriptions
	SetupHostSubscriptions(instanceID, whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"id":     graphql.String(instanceID),
		"status": graphql.String("DRAINING"),
	}

	// Verify that the "variables" maps are deep equal for the first subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables = map[string]interface{}{
		"instance_id": graphql.String(instanceID),
		"status":      graphql.String("ALLOCATED"),
	}

	// Verify that the "variables" maps are deep equal for the second subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}

func TestSetupScalingSubscriptions(t *testing.T) {
	whistClient := &mockWhistClient{}

	// Create the scaling service specific subscriptions
	SetupScalingSubscriptions(whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 1 {
		t.Errorf("Expected subscriptions lenght to be 1, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"status": graphql.String("DRAINING"),
	}

	// Verify that the "variables" maps are deep equal for the first subscription

	if !reflect.DeepEqual(variables, whistClient.Subscriptions[0].Variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}
}

// TODO: Add test to subscriptions start function once scaling service is done.
