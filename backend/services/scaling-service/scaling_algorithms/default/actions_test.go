package scaling_algorithms

import (
	"context"
	"testing"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

type mockDBClient struct{}

func (db *mockDBClient) QueryInstance(context.Context, *subscriptions.GraphQLClient, string) (subscriptions.WhistInstances, error) {
	return subscriptions.WhistInstances{}, nil
}

func (db *mockDBClient) QueryInstancesByStatusOnRegion(context.Context, *subscriptions.GraphQLClient, string) (subscriptions.WhistInstances, error) {
	return subscriptions.WhistInstances{}, nil
}

func (db *mockDBClient) QueryInstancesByImage(context.Context, *subscriptions.GraphQLClient, string) (subscriptions.WhistInstances, error) {
	return subscriptions.WhistInstances{}, nil
}

func (db *mockDBClient) InsertInstances(context.Context, *subscriptions.GraphQLClient, []subscriptions.Instance) (int, error) {
	return 0, nil
}

func (db *mockDBClient) UpdateInstance(context.Context, *subscriptions.GraphQLClient, map[string]interface{}) (int, error) {
	return 0, nil
}

func (db *mockDBClient) DeleteInstance(context.Context, *subscriptions.GraphQLClient, string) (int, error) {
	return 0, nil
}

func (db *mockDBClient) QueryImage(context.Context, *subscriptions.GraphQLClient, string, string) (subscriptions.WhistImages, error) {
	return subscriptions.WhistImages{}, nil
}

func (db *mockDBClient) InsertImages(context.Context, *subscriptions.GraphQLClient, []subscriptions.Image) (int, error) {
	return 0, nil
}

func (db *mockDBClient) UpdateImage(context.Context, *subscriptions.GraphQLClient, subscriptions.Image) (int, error) {
	return 0, nil
}

func TestVerifyInstanceScaleDown(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	testAlgorithm := &DefaultScalingAlgorithm{
		Region: "test-region",
	}
	testDBClient := &mockDBClient{}
	testAlgorithm.CreateBuffer()
	testAlgorithm.CreateDBClient((*dbclient.DBClient)(testDBClient))

	err := testAlgorithm.VerifyCapacity(context, ScalingEvent{Region: "test-region"})
	if err != nil {
		t.Errorf("Failed while testing verify capacity action. Err: %v", err)
	}
}

func TestVerifyCapacity(t *testing.T) {}

func TestScaleDownIfNecessary(t *testing.T) {}

func TestScaleUpIfNecessary(t *testing.T) {}

func TestUpgradeImage(t *testing.T) {}
