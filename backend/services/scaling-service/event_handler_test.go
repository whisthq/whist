package main

import (
	"context"
	"encoding/json"
	"os"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	algos "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

type mockSubscriptionClient struct {
	Initialized     bool
	Started         bool
	Params          subscriptions.HasuraParams
	Subscriptions   []subscriptions.HasuraSubscription
	SubscriptionIDs []string
}

func (mc *mockSubscriptionClient) Initialize() error {
	mc.Initialized = true
	return nil
}

func (mc *mockSubscriptionClient) GetSubscriptions() []subscriptions.HasuraSubscription {
	return mc.Subscriptions
}

func (mc *mockSubscriptionClient) SetSubscriptions(subs []subscriptions.HasuraSubscription) {
	mc.Subscriptions = subs
}

func (mc *mockSubscriptionClient) GetSubscriptionIDs() []string {
	return mc.SubscriptionIDs
}

func (mc *mockSubscriptionClient) SetSubscriptionsIDs(ids []string) {
	mc.SubscriptionIDs = ids
}

func (mc *mockSubscriptionClient) GetParams() subscriptions.HasuraParams {
	return mc.Params
}

func (mc *mockSubscriptionClient) SetParams(params subscriptions.HasuraParams) {
	mc.Params = params
}

func (mc *mockSubscriptionClient) Subscribe(query subscriptions.GraphQLQuery, variables map[string]interface{}, event subscriptions.SubscriptionEvent, fn subscriptions.Handlerfn, subcscriptionEvents chan subscriptions.SubscriptionEvent) (string, error) {
	return "test-subscription-id", nil
}

func (mc *mockSubscriptionClient) Run(*sync.WaitGroup) {
	mc.Started = true
}

func (mc *mockSubscriptionClient) Close([]string) error {
	return nil
}

// TestStartDatabaseSubscriptions tests that all parts of the subscriptions package
// work with the database subscriptions used in the scaling service.
func TestStartDatabaseSubscriptions(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	goroutinetracker := &sync.WaitGroup{}
	subscriptionEvents := make(chan subscriptions.SubscriptionEvent)
	mockClient := &mockSubscriptionClient{}
	mockConfigClient := &mockSubscriptionClient{}

	// Override enabled condition
	subscriptions.Enabled = true

	// Start database subscriptions with mock client
	StartDatabaseSubscriptions(ctx, goroutinetracker, subscriptionEvents, mockClient, mockConfigClient)

	testScalingSubscriptions := []subscriptions.HasuraSubscription{
		{
			Query: subscriptions.QueryInstancesByStatus,
			Variables: map[string]interface{}{
				"status": graphql.String("DRAINING"),
			},
			Result: subscriptions.InstanceEvent{
				Instances: []subscriptions.Instance{},
			},
			Handler: subscriptions.InstancesStatusHandler,
		},
	}

	// First, verify that the scaling subscriptions were set correctly
	if len(mockClient.GetSubscriptions()) != 1 {
		t.Fatalf("Got %v database subscriptions, want 1", len(mockClient.GetSubscriptions()))
	}

	gotSubscription := mockClient.GetSubscriptions()[0]
	ok := reflect.DeepEqual(gotSubscription.Query, testScalingSubscriptions[0].Query)
	if !ok {
		t.Errorf("Scaling service subscriptions query not set correctly. Got %T, want %T", gotSubscription.Query, testScalingSubscriptions[0].Query)
	}
	ok = reflect.DeepEqual(gotSubscription.Variables, testScalingSubscriptions[0].Variables)
	if !ok {
		t.Errorf("Scaling service subscriptions variables not set correctly. Got %v, want %v", gotSubscription.Variables, testScalingSubscriptions[0].Variables)
	}
	ok = reflect.DeepEqual(gotSubscription.Result, testScalingSubscriptions[0].Result)
	if !ok {
		t.Errorf("Scaling service subscriptions result not set correctly. Got %v, want %v", gotSubscription.Result, testScalingSubscriptions[0].Result)
	}

	// Check if the client was initialized
	if !mockClient.Initialized {
		t.Errorf("Subscriptions client was not initialized.")
	}

	// Verify that the client subscribed
	if len(mockClient.GetSubscriptionIDs()) != 1 {
		t.Fatalf("Got the wrong number of subscription IDs. Got %v, want 1", len(mockClient.SubscriptionIDs))
	}

	// Check if the subscription id is correct
	ok = reflect.DeepEqual(mockClient.GetSubscriptionIDs()[0], "test-subscription-id")
	if !ok {
		t.Errorf("Got the wrong subscription id. Got %v, want %v", mockClient.GetSubscriptionIDs()[0], "test-subscription-id")
	}

	// Finally, verify that the client runs the subscriptions
	if !mockClient.Started {
		t.Errorf("Failed to run subscription client.")
	}
}

func TestStartSchedulerEvents(t *testing.T) {
	scheduledEvents := make(chan algos.ScalingEvent)
	after := time.Duration(1 * time.Second)
	StartSchedulerEvents(scheduledEvents, 1, after)

	// Block scheduledEvents channel until we have received
	// the scheduled event, timeout otherwise.
	select {
	case event := <-scheduledEvents:
		event.ID = utils.PlaceholderTestUUID().String()
		testEvent := algos.ScalingEvent{
			ID:   utils.PlaceholderTestUUID().String(),
			Type: "SCHEDULED_SCALE_DOWN_EVENT",
		}
		ok := reflect.DeepEqual(event, testEvent)

		if !ok {
			t.Errorf("Got malformed scheduled event. Got %v, want %v", event, testEvent)
		}
	case <-time.After(2 * time.Second):
		t.Errorf("Timed out waiting for scheduled event.")
	}

}

func TestStartDeploy(t *testing.T) {
	imageRegionMap := map[string]interface{}{
		"us-east-1": "test-image-id",
		"us-east-2": "test-image-id-2",
	}

	// Encode image region map
	imageMapBytes, err := json.Marshal(imageRegionMap)
	if err != nil {
		t.Errorf("Error encoding region image map. Err: %v", err)
	}

	// Write file
	err = os.WriteFile("images.json", imageMapBytes, 0777)
	if err != nil {
		t.Errorf("Failed to write images.json file. Err: %v", err)
	}

	// Call StartDeploy function
	scheduledEvents := make(chan algos.ScalingEvent, 100)
	StartDeploy(scheduledEvents)

	// Verify that event has contents of file
	select {
	case event := <-scheduledEvents:
		event.ID = utils.PlaceholderTestUUID().String()
		testEvent := algos.ScalingEvent{
			ID:   utils.PlaceholderTestUUID().String(),
			Data: imageRegionMap,
			Type: "SCHEDULED_IMAGE_UPGRADE_EVENT",
		}
		ok := reflect.DeepEqual(event, testEvent)

		if !ok {
			t.Errorf("Got malformed scheduled event. Got %v, want %v", event, testEvent)
		}
	case <-time.After(2 * time.Second):
		t.Errorf("Timed out waiting for scheduled event.")
	}
}

func TestGetScalingAlgorithm(t *testing.T) {
	algorithmByRegionMap := &sync.Map{}
	testRegions := []string{
		"us-east-1-test",
	}
	testAlgorithm := &algos.DefaultScalingAlgorithm{
		Region: "us-east-1-test",
	}

	// Load default scaling algorithm for all enabled regions.
	for _, region := range testRegions {
		name := utils.Sprintf("default-sa-%s", region)
		algorithmByRegionMap.Store(name, &algos.DefaultScalingAlgorithm{
			Region: region,
		})
	}

	algorithm := getScalingAlgorithm(algorithmByRegionMap, algos.ScalingEvent{
		Region: "us-east-1-test",
	})

	ok := reflect.DeepEqual(algorithm, testAlgorithm)
	if !ok {
		t.Errorf("Failed to get correct scaling algorithm. Got %v, want %v", algorithm, testAlgorithm)
	}
}
