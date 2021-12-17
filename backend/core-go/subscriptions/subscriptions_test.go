package subscriptions

import (
	"context"
	"reflect"
	"sync"
	"testing"

	"github.com/google/uuid"
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
		"instanceName": "test-instance-name",
		"status":       "DRAINING",
	}

	// Create different tests for the instance status handler,
	// verify if it returns the appropiate response
	var instanceTests = []struct {
		testName string
		event    InstanceEvent
		want     bool
	}{
		{"Empty event", InstanceEvent{InstanceInfo: []Instance{}}, false},
		{"Wrong status event", InstanceEvent{
			InstanceInfo: []Instance{
				{InstanceName: "test-instance-name", Status: "PRE_CONNECTION"},
			},
		}, false},
		{"Correct status event", InstanceEvent{
			InstanceInfo: []Instance{
				{InstanceName: "test-instance-name", Status: "DRAINING"},
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
		"instanceName": "test-instance-name",
		"status":       "ALLOCATED",
	}

	// Create different tests for the mandelbox allocated handler,
	// verify if it returns the appropiate response
	var mandelboxTests = []struct {
		testName string
		event    MandelboxEvent
		want     bool
	}{
		{"Empty event", MandelboxEvent{MandelboxInfo: []Mandelbox{}}, false},
		{"Wrong instance name event", MandelboxEvent{
			MandelboxInfo: []Mandelbox{
				{InstanceName: "test-instance-name-2", Status: "EXITED"},
			},
		}, false},
		{"Correct status event", MandelboxEvent{
			MandelboxInfo: []Mandelbox{
				{InstanceName: "test-instance-name", Status: "ALLOCATED"},
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
func TestMandelboxStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"status": "ALLOCATED",
	}

	// Create different tests for the mandelbox status handler,
	// verify if it returns the appropiate response
	var mandelboxTests = []struct {
		testName string
		event    MandelboxEvent
		want     bool
	}{
		{"Empty event", MandelboxEvent{MandelboxInfo: []Mandelbox{}}, false},
		{"Wrong status event", MandelboxEvent{
			MandelboxInfo: []Mandelbox{
				{Status: "EXITED"},
			},
		}, false},
		{"Correct status event", MandelboxEvent{
			MandelboxInfo: []Mandelbox{
				{Status: "ALLOCATED"},
			},
		}, true},
	}

	for _, tt := range mandelboxTests {
		testname := tt.testName
		t.Run(testname, func(t *testing.T) {
			got := MandelboxStatusHandler(tt.event, variables)

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestSetupHostSubscriptions(t *testing.T) {
	instanceName := "test-instance-name"
	whistClient := &mockWhistClient{}

	// Create the host service specific subscriptions
	SetupHostSubscriptions(instanceName, whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"instanceName": instanceName,
		"status":       "DRAINING",
	}

	// Verify that the "variables" maps are deep equal for the first subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = "ALLOCATED"

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

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	// Create a fake variables map that matches the host subscriptions variable map
	var variables = map[string]interface{}{
		"status": "ALLOCATED",
	}

	// Verify that the "variables" maps are deep equal for the first subscription

	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = "EXITED"

	// Verify that the "variables" maps are deep equal for the second subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}
func TestStart(t *testing.T) {
	whistClient := &mockWhistClient{}
	instanceName := "test-instance-name"

	// Setup context, goroutine tracker and event channel
	ctx, cancel := context.WithCancel(context.Background())
	tracker := sync.WaitGroup{}
	subscriptionEvents := make(chan SubscriptionEvent, 10)

	// Create the host service specific subscriptions
	SetupHostSubscriptions(instanceName, whistClient)

	// Start the subscriptions
	Start(whistClient, ctx, &tracker, subscriptionEvents)

	// Verify that the results are sent through the appropiate channel,
	// and correspond to the first host service subscription
	event := <-subscriptionEvents
	instanceResult := event.(InstanceEvent)
	instance := instanceResult.InstanceInfo[0]

	// Check that the result has the correct information
	if instance.InstanceName != instanceName {
		t.Errorf("Expected instance name %v, got %v", instanceName, instance.InstanceName)
	}

	if instance.Status != "DRAINING" {
		t.Errorf("Expected status %v, got %v", "DRAINING", instance.Status)
	}

	// Verify that the results are sent through the appropiate channel,
	// and correspond to the second host service subscription
	event = <-subscriptionEvents
	mandelboxResult := event.(MandelboxEvent)
	mandelbox := mandelboxResult.MandelboxInfo[0]

	// Check that the result has the correct information
	if mandelbox.InstanceName != instanceName {
		t.Errorf("Expected instance name %v, got %v", instanceName, mandelbox.InstanceName)
	}

	if mandelbox.Status != "ALLOCATED" {
		t.Errorf("Expected status %v, got %v", "ALLOCATED", mandelbox.Status)
	}

	// Cancel context and terminate client.
	cancel()
}
