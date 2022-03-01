package scaling_algorithms

import (
	"context"
	"testing"
	"time"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var testInstances subscriptions.WhistInstances

// mockDBClient is used to test all database interactions
type mockDBClient struct{}

func (db *mockDBClient) QueryInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, instanceID string) (subscriptions.WhistInstances, error) {
	return testInstances, nil
}

func (db *mockDBClient) QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, status string, region string) (subscriptions.WhistInstances, error) {
	return subscriptions.WhistInstances{}, nil
}

func (db *mockDBClient) QueryInstancesByImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, imageID string) (subscriptions.WhistInstances, error) {
	return subscriptions.WhistInstances{}, nil
}

func (db *mockDBClient) InsertInstances(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, insertParams []subscriptions.Instance) (int, error) {
	return 0, nil
}

func (db *mockDBClient) UpdateInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, updateParams map[string]interface{}) (int, error) {
	return 1, nil
}

func (db *mockDBClient) DeleteInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, instanceID string) (int, error) {
	testInstances = subscriptions.WhistInstances{}
	return 1, nil
}

func (db *mockDBClient) QueryImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, provider string, region string) (subscriptions.WhistImages, error) {
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

func (db *mockDBClient) InsertImages(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, insertParams []subscriptions.Image) (int, error) {
	return 0, nil
}

func (db *mockDBClient) UpdateImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, image subscriptions.Image) (int, error) {
	return 0, nil
}

// mockHostHandler is used to test all interactions with cloud providers
type mockHostHandler struct{}

func (mh *mockHostHandler) Initialize(region string) error {
	return nil
}

func (mh *mockHostHandler) SpinUpInstances(scalingCtx context.Context, numInstances int32, imageID string) (createdInstances []subscriptions.Instance, err error) {
	return []subscriptions.Instance{}, nil
}

func (mh *mockHostHandler) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) (terminatedInstances []subscriptions.Instance, err error) {
	return []subscriptions.Instance{}, nil
}

func (mh *mockHostHandler) WaitForInstanceTermination(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	return nil
}

func (mh *mockHostHandler) WaitForInstanceReady(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
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

	// Pass a mock db client
	testAlgorithm.CreateDBClient((*dbclient.DBClient)(testDBClient))

	// Create test instance that will be used for testing
	instanceToTest = subscriptions.WhistInstances{
		{
			ID:       "test-verify-scale-down-instance",
			Provider: "AWS",
		},
	}

	err := testAlgorithm.VerifyInstanceScaleDown(context, ScalingEvent{Region: "test-region"}, instanceToTest)
	if err != nil {
		t.Errorf("Failed while testing verify scale down action. Err: %v", err)
	}
}

func TestVerifyCapacity(t *testing.T) {}

func TestScaleDownIfNecessary(t *testing.T) {}

func TestScaleUpIfNecessary(t *testing.T) {}

func TestUpgradeImage(t *testing.T) {}
