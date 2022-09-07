package subscriptions

import (
	"context"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
)

func TestInstanceStatusHandler(t *testing.T) {
	var variables = map[string]interface{}{
		"id":     graphql.String("test-instance-id"),
		"status": graphql.String(dbdriver.InstanceStatusDraining),
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
			Instances: []Instance{{ID: ""}},
		}, false},
		{"Correct status event", InstanceEvent{
			Instances: []Instance{
				{ID: "test-instance-id", Status: string(dbdriver.InstanceStatusDraining)},
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
		"status":      graphql.String(dbdriver.MandelboxStatusAllocated),
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
				{InstanceID: "test-instance-id", Status: string(dbdriver.MandelboxStatusAllocated)},
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
		"id": graphql.String(instanceID),
	}

	// Verify that the "variables" maps are deep equal for the first subscription
	if !reflect.DeepEqual(whistClient.Subscriptions[0].Variables, variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}

	variables = map[string]interface{}{
		"instance_id": graphql.String(instanceID),
		"status":      graphql.String(dbdriver.MandelboxStatusAllocated),
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
		"status": graphql.String(dbdriver.InstanceStatusDraining),
	}

	// Verify that the "variables" maps are deep equal for the first subscription

	if !reflect.DeepEqual(variables, whistClient.Subscriptions[0].Variables) {
		t.Errorf("Expected variable map to be %v, got: %v", whistClient.Subscriptions[0].Variables, variables)
	}
}

func TestSubscriptionLifecycle(t *testing.T) {
	events := make(chan SubscriptionEvent)
	tracker := &sync.WaitGroup{}
	msg := randomID()

	server := subscription_setupServer()
	client, subscriptionClient := subscription_setupClients(nil)

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer server.Shutdown(ctx)
	defer cancel()

	var sub struct {
		HelloSaid struct {
			ID      graphql.String
			Message graphql.String `graphql:"msg" json:"msg"`
		} `graphql:"helloSaid" json:"helloSaid"`
	}

	err := runServerAndSubscribe(ctx, server, subscriptionClient, sub, events)
	if err != nil {
		t.Fatalf("failed to run server or subscribe: %s", err)
	}

	go func(tracker *sync.WaitGroup) {
		subscriptionClient.Run(tracker)
	}(tracker)
	defer subscriptionClient.Hasura.Close()

	err = triggerSubscription(client, msg)
	if err != nil {
		t.Fatalf("failed to trigger subscription: %s", err)
	}

	e := <-events

	// Check that the result is the expected
	subRes := e.(map[string]interface{})["helloSaid"].(map[string]interface{})
	if subRes["msg"] != msg {
		t.Fatalf("subscription message does not match. got: %s, want: %s", subRes["msg"], msg)
	}
}

func TestClose(t *testing.T) {
	// Use a non-blocking channel to test subscription closing
	events := make(chan SubscriptionEvent, 1)
	tracker := &sync.WaitGroup{}
	msg := randomID()

	server := subscription_setupServer()
	client, subscriptionClient := subscription_setupClients(nil)

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer server.Shutdown(ctx)
	defer cancel()

	var sub struct {
		HelloSaid struct {
			ID      graphql.String
			Message graphql.String `graphql:"msg" json:"msg"`
		} `graphql:"helloSaid" json:"helloSaid"`
	}

	err := runServerAndSubscribe(ctx, server, subscriptionClient, sub, events)
	if err != nil {
		t.Fatalf("failed to run server or subscribe: %s", err)
	}

	go func(tracker *sync.WaitGroup) {
		subscriptionClient.Run(tracker)
	}(tracker)

	// Wait for the client to start subscriptions before closing
	time.Sleep(1 * time.Second)

	// Close the client connection prematurely, so
	// no event should be sent on the events channel.
	err = subscriptionClient.Close()
	if err != nil {
		t.Errorf("failed to close subscriptions client: %s", err)
	}

	err = triggerSubscription(client, msg)
	if err != nil {
		t.Fatalf("failed to trigger subscription: %s", err)
	}

	select {
	case e := <-events:
		if e != nil {
			t.Errorf("expected nil event, got: %s", e)
		}

	case <-time.After(3 * time.Second):
		// The test should time out because the subscriptions
		// weere closed prematurely./
		return
	}

}
