package scaling_algorithms

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var testInstances subscriptions.WhistInstances

// mockDBClient is used to test all database interactions
type mockDBClient struct{}

func (db *mockDBClient) QueryInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (subscriptions.WhistInstances, error) {
	return testInstances, nil
}

func (db *mockDBClient) QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, status string, region string) (subscriptions.WhistInstances, error) {
	return testInstances, nil
}

func (db *mockDBClient) QueryInstancesByImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, imageID string) (subscriptions.WhistInstances, error) {
	return testInstances, nil
}

func (db *mockDBClient) InsertInstances(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Instance) (int, error) {
	for _, instance := range insertParams {
		testInstances = append(testInstances, struct {
			ID                graphql.String                 `graphql:"id"`
			Provider          graphql.String                 `graphql:"provider"`
			Region            graphql.String                 `graphql:"region"`
			ImageID           graphql.String                 `graphql:"image_id"`
			ClientSHA         graphql.String                 `graphql:"client_sha"`
			IPAddress         string                         `graphql:"ip_addr"`
			Type              graphql.String                 `graphql:"instance_type"`
			RemainingCapacity graphql.Int                    `graphql:"remaining_capacity"`
			Status            graphql.String                 `graphql:"status"`
			CreatedAt         time.Time                      `graphql:"created_at"`
			UpdatedAt         time.Time                      `graphql:"updated_at"`
			Mandelboxes       subscriptions.WhistMandelboxes `graphql:"mandelboxes"`
		}{
			ID:                graphql.String(instance.ID),
			Provider:          graphql.String(instance.Provider),
			ImageID:           graphql.String(instance.ImageID),
			Type:              graphql.String(instance.Type),
			RemainingCapacity: graphql.Int(instance.RemainingCapacity),
			Status:            graphql.String(instance.Status),
		})
	}

	return len(testInstances), nil
}

func (db *mockDBClient) UpdateInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams map[string]interface{}) (int, error) {
	for index, instance := range testInstances {
		if instance.ID == updateParams["id"] {
			updatedInstance := struct {
				ID                graphql.String                 `graphql:"id"`
				Provider          graphql.String                 `graphql:"provider"`
				Region            graphql.String                 `graphql:"region"`
				ImageID           graphql.String                 `graphql:"image_id"`
				ClientSHA         graphql.String                 `graphql:"client_sha"`
				IPAddress         string                         `graphql:"ip_addr"`
				Type              graphql.String                 `graphql:"instance_type"`
				RemainingCapacity graphql.Int                    `graphql:"remaining_capacity"`
				Status            graphql.String                 `graphql:"status"`
				CreatedAt         time.Time                      `graphql:"created_at"`
				UpdatedAt         time.Time                      `graphql:"updated_at"`
				Mandelboxes       subscriptions.WhistMandelboxes `graphql:"mandelboxes"`
			}{
				ID:                updateParams["id"].(graphql.String),
				Provider:          instance.Provider,
				ImageID:           instance.ImageID,
				Type:              instance.Type,
				Status:            updateParams["status"].(graphql.String),
				RemainingCapacity: instance.RemainingCapacity,
			}
			testInstances[index] = updatedInstance
		}
	}
	return len(testInstances), nil
}

func (db *mockDBClient) DeleteInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (int, error) {
	testInstances = subscriptions.WhistInstances{}
	return 0, nil
}

func (db *mockDBClient) QueryImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, provider string, region string) (subscriptions.WhistImages, error) {
	return subscriptions.WhistImages{
		{
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id",
			ClientSHA: "test-sha",
			UpdatedAt: time.Now(),
		},
	}, nil
}

func (db *mockDBClient) InsertImages(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Image) (int, error) {
	return 0, nil
}

func (db *mockDBClient) UpdateImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, image subscriptions.Image) (int, error) {
	return 0, nil
}

// mockHostHandler is used to test all interactions with cloud providers
type mockHostHandler struct{}

func (mh *mockHostHandler) Initialize(region string) error {
	return nil
}

func (mh *mockHostHandler) SpinUpInstances(scalingCtx context.Context, numInstances int32, imageID string) (createdInstances []subscriptions.Instance, err error) {
	var newInstances []subscriptions.Instance
	for i := 0; i < int(numInstances); i++ {
		newInstances = append(newInstances, subscriptions.Instance{
			ID:       "test-scale-up-instance",
			Provider: "AWS",
			ImageID:  imageID,
			Type:     "g4dn.2xlarge",
			Status:   "PRE_CONNECTION",
		})
	}

	return newInstances, nil
}

func (mh *mockHostHandler) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) (terminatedInstances []subscriptions.Instance, err error) {
	return []subscriptions.Instance{}, nil
}

func (mh *mockHostHandler) WaitForInstanceTermination(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	// Remove test instances to mock them shutting down
	testInstances = subscriptions.WhistInstances{}
	return nil
}

func (mh *mockHostHandler) WaitForInstanceReady(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	return nil
}

// mockHostHandler is used to test all interactions with hasura

type mockGraphQLClient struct{}

func (mg *mockGraphQLClient) Initialize() error {
	return nil
}

func (mg *mockGraphQLClient) Query(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

func (mg *mockGraphQLClient) Mutate(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

func TestVerifyInstanceScaleDown(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create and initialize a test scaling algorithm
	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}

	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:       "test-verify-scale-down-instance",
			Provider: "AWS",
		},
	}

	// For this test, we start with a test instance that is draining. It should be
	// removed, and another instance should be started to match required capacity.
	err := testAlgorithm.VerifyInstanceScaleDown(context, ScalingEvent{Region: "test-region"}, subscriptions.Instance{
		ID: "test-verify-scale-down-instance",
	})
	if err != nil {
		t.Errorf("Failed while testing verify scale down action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	ok := reflect.DeepEqual(testInstances, subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	})
	if !ok {
		t.Errorf("Failed to verify instance scale down. There were no instances scaled up after terminating test instance.")
	}
}

func TestVerifyCapacity(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create and initialize a test scaling algorithm
	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}

	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)

	testInstances = subscriptions.WhistInstances{}

	// For this test, we will start with no capacity to check if
	// the function properly starts instances.
	err := testAlgorithm.VerifyCapacity(context, ScalingEvent{Region: "test-region"})
	if err != nil {
		t.Errorf("Failed while testing verify capacity action. Err: %v", err)
	}

	// Check that an instance was scaled up
	ok := reflect.DeepEqual(testInstances, subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	})
	if !ok {
		t.Errorf("Failed to verify instance scale down. There were no instances scaled up.")
	}
}

func TestScaleDownIfNecessary(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create and initialize a test scaling algorithm
	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}

	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:                "test-scale-down-instance-1",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-down-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	}

	// For this test, we start with more instances than desired, so we can check
	// if the scale down correctly terminates free instances.
	err := testAlgorithm.ScaleDownIfNecessary(context, ScalingEvent{Region: "test-region"})
	if err != nil {
		t.Errorf("Failed while testing scale down action. Err: %v", err)
	}

	t.Logf("Scale down exited with %v", testInstances)

	// Check that free instances were scaled down
	ok := reflect.DeepEqual(testInstances, subscriptions.WhistInstances{
		{
			ID:                "test-scale-down-instance-1",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "DRAINING",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "DRAINING",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-down-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	})
	if !ok {
		t.Errorf("Instances were not scaled down correctly.")
	}
}

func TestScaleUpIfNecessary(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create and initialize a test scaling algorithm
	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}

	testInstances = subscriptions.WhistInstances{}
	testInstancesToScale := 3

	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)

	// For this test, try to scale up instances and check if they are
	// successfully added to the database with the correct data.
	err := testAlgorithm.ScaleUpIfNecessary(testInstancesToScale, context, ScalingEvent{Region: "test-region"}, "test-image-id-scale-up")
	if err != nil {
		t.Errorf("Failed while testing scale up action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	ok := reflect.DeepEqual(testInstances, subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	})
	if !ok {
		t.Errorf("Did not scale up instances correctly while testing scale up action.")
	}
}

func TestUpgradeImage(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create and initialize a test scaling algorithm
	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}

	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	}

	// For this test, start an image upgrade, check that instances with the new image
	// are created and that old instances are left untouched.
	err := testAlgorithm.UpgradeImage(context, ScalingEvent{Region: "test-region"}, "test-image-id-new")
	if err != nil {
		t.Errorf("Failed while testing scale up action. Err: %v", err)
	}

	t.Logf("Ended up with isntaces %v", testInstances)
	// Check that an instance was scaled up after the test instance was removed
	ok := reflect.DeepEqual(testInstances, subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-new",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 3,
		},
	})
	if !ok {
		t.Errorf("Did not scale up instances correctly while testing scale up action.")
	}
}
