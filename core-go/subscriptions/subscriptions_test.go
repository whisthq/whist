package subscriptions

import (
	"context"
	"reflect"
	"sync"
	"testing"

	"github.com/fractal/fractal/core-go/metadata/aws"
)

type mockWhistClient struct {
	Params          HasuraParams
	Subscriptions   []HasuraSubscription
	SubscriptionIDs []string
}

func (cl *mockWhistClient) Initialize()                          {}
func (cl *mockWhistClient) Run(goroutineTracker *sync.WaitGroup) {}

func (cl *mockWhistClient) Subscribe(query GraphQLQuery, variables map[string]interface{}, result SubscriptionEvent,
	conditionFn handlerfn, subscriptionEvents chan SubscriptionEvent) (string, error) {
	return "", nil
}

func (cl *mockWhistClient) Close(subscriptionIDs []string) error {
	return nil
}

func TestInstanceStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"instanceName": "test-instance-name",
		"status":       "DRAINING",
	}

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

func TestInitializeWhistHasuraClientOnLocalEnv(t *testing.T) {
	whistClient := mockWhistClient{}
	err := InitializeWhistHasuraClient(&whistClient)

	if err != nil {
		t.Errorf("Expected err to be nil, got: %v", err)
	}
}

func TestSetupHostSubscriptions(t *testing.T) {
	instanceName := "test-instance-name"
	whistClient := WhistClient{}

	SetupHostSubscriptions(aws.InstanceName(instanceName), &whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	var variables = map[string]interface{}{
		"instanceName": instanceName,
		"status":       "DRAINING",
	}

	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = "ALLOCATED"

	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}

func TestSetupScalingSubscriptions(t *testing.T) {
	whistClient := WhistClient{}

	SetupScalingSubscriptions(&whistClient)

	if whistClient.Subscriptions == nil {
		t.Errorf("Got nil subscriptions")
	}

	if len(whistClient.Subscriptions) != 2 {
		t.Errorf("Expected subscriptions lenght to be 2, got: %v", len(whistClient.Subscriptions))
	}

	var variables = map[string]interface{}{
		"status": "ALLOCATED",
	}

	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables["status"] = "EXITED"

	if !reflect.DeepEqual(whistClient.Subscriptions[1].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[1].Variables, variables)
	}
}
func TestStart(t *testing.T) {
	whistClient := WhistClient{}
	ctx := context.Background()
	tracker := sync.WaitGroup{}
	subscriptionEvents := make(chan SubscriptionEvent)

	Start(&whistClient, ctx, &tracker, subscriptionEvents)
}
