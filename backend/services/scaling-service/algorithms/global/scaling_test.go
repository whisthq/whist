package global

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

func TestScaleDownIfNecessary(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:                "test-scale-down-instance-1",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
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

	// For this test, we start with more instances than desired, so we can check
	// if the scale down correctly terminates free instances.
	err := testAlgorithm.ScaleDownIfNecessary(context, algorithms.ScalingEvent{Region: "test-region"})
	if err != nil {
		t.Errorf("Failed while testing scale down action. Err: %v", err)
	}

	// Check that free instances were scaled down
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-scale-down-instance-1",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "DRAINING",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "DRAINING",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			ClientSHA:         "test-sha-dev",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
	}
	ok := reflect.DeepEqual(testInstances, expectedInstances)
	if !ok {
		t.Errorf("Instances were not scaled down correctly. Expected %v, got %v", expectedInstances, testInstances)
	}
}

func TestScaleUpIfNecessary(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	testInstances = subscriptions.WhistInstances{}
	testInstancesToScale := 3

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

	// For this test, try to scale up instances and check if they are
	// successfully added to the database with the correct data.
	err := testAlgorithm.ScaleUpIfNecessary(testInstancesToScale, context, algorithms.ScalingEvent{Region: "test-region"}, subscriptions.Image{
		ImageID:   "test-image-id-scale-up",
		ClientSHA: "test-sha-dev",
	})
	if err != nil {
		t.Errorf("Failed while testing scale up action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			ClientSHA:         "test-sha-dev",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			ClientSHA:         "test-sha-dev",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			ClientSHA:         "test-sha-dev",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
	}
	ok := reflect.DeepEqual(testInstances, expectedInstances)
	if !ok {
		t.Errorf("Did not scale up instances correctly while testing scale up action. Expected %v, got %v", expectedInstances, testInstances)
	}
}
