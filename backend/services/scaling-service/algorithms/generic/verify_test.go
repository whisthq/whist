package generic

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

func TestVerifyInstanceScaleDown(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:       "test-verify-scale-down-instance",
			Provider: "AWS",
		},
	}

	// Set the current image for testing
	testImages = subscriptions.WhistImages{
		struct {
			Provider  graphql.String `graphql:"provider"`
			Region    graphql.String `graphql:"region"`
			ImageID   graphql.String `graphql:"image_id"`
			ClientSHA graphql.String `graphql:"client_sha"`
			UpdatedAt time.Time      `graphql:"updated_at"`
		}{
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id",
			ClientSHA: "test-sha-dev",
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	// For this test, we start with a test instance that is draining. It should be
	// removed, and another instance should be started to match required capacity.
	err := testAlgorithm.VerifyInstanceScaleDown(context, algorithms.ScalingEvent{Region: "test-region"}, subscriptions.Instance{
		ID: "test-verify-scale-down-instance",
	})
	if err != nil {
		t.Errorf("Failed while testing verify scale down action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
	}
	ok := reflect.DeepEqual(testInstances, expectedInstances)
	if !ok {
		t.Errorf("Failed to verify instance scale down. Expected %v, got %v", expectedInstances, testInstances)
	}
}

func TestVerifyCapacity(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	testInstances = subscriptions.WhistInstances{}

	// Set the current image for testing
	testImages = subscriptions.WhistImages{
		struct {
			Provider  graphql.String `graphql:"provider"`
			Region    graphql.String `graphql:"region"`
			ImageID   graphql.String `graphql:"image_id"`
			ClientSHA graphql.String `graphql:"client_sha"`
			UpdatedAt time.Time      `graphql:"updated_at"`
		}{
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id",
			ClientSHA: "test-sha-dev",
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	// For this test, we will start with no capacity to check if
	// the function properly starts instances.
	err := testAlgorithm.VerifyCapacity(context, algorithms.ScalingEvent{Region: "test-region"})
	if err != nil {
		t.Errorf("Failed while testing verify capacity action. Err: %v", err)
	}

	// Check that an instance was scaled up
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
	}
	ok := reflect.DeepEqual(testInstances, expectedInstances)
	if !ok {
		t.Errorf("Failed to verify instance scale down. Expected %v, got %v", expectedInstances, testInstances)
	}
}
