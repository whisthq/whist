// This file defines mock types and methods to test the
// different actions on the scaling algorithm.

package scaling_algorithms

import (
	"context"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/whisthq/whist/backend/services/scaling-service/config"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var (
	testInstances   []subscriptions.Instance
	testImages      []subscriptions.Image
	testMandelboxes []subscriptions.Mandelbox
	testAlgorithm   *DefaultScalingAlgorithm
	testLock        *sync.Mutex
)

// mockDBClient is used to test all database interactions
type mockDBClient struct{}

func (db *mockDBClient) QueryInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) ([]subscriptions.Instance, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var result []subscriptions.Instance
	for _, instance := range testInstances {
		if string(instance.ID) == instanceID {
			result = append(result, instance)
		}
	}

	return result, nil
}

func (db *mockDBClient) QueryInstanceWithCapacity(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, region string) ([]subscriptions.Instance, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var instancesWithCapacity []subscriptions.Instance
	for _, instance := range testInstances {
		if string(instance.Region) == region && instance.RemainingCapacity > 0 {
			instancesWithCapacity = append(instancesWithCapacity, instance)
		}
	}
	return instancesWithCapacity, nil
}

func (db *mockDBClient) QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, status string, region string) ([]subscriptions.Instance, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var result []subscriptions.Instance
	for _, instance := range testInstances {
		if string(instance.Status) == status &&
			string(instance.Region) == region {
			result = append(result, instance)
		}
	}

	return result, nil
}

func (db *mockDBClient) QueryInstancesByImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, imageID string) ([]subscriptions.Instance, error) {
	testLock.Lock()
	defer testLock.Unlock()

	return testInstances, nil
}

func (db *mockDBClient) InsertInstances(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Instance) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()
	testInstances = append(testInstances, insertParams...)

	return len(insertParams), nil
}

func (db *mockDBClient) UpdateInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams subscriptions.Instance) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var updated int
	for index, instance := range testInstances {
		if string(instance.ID) == updateParams.ID {
			testInstances[index] = updateParams
			updated++
		}
	}
	return updated, nil
}

func (db *mockDBClient) DeleteInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var i int
	for index, instance := range testInstances {
		if string(instance.ID) == instanceID {
			i = index
		}
	}

	// Delete instance from testInstances slice
	testInstances[i] = testInstances[len(testInstances)-1]
	testInstances = testInstances[:len(testInstances)-1]

	return 0, nil
}

func (db *mockDBClient) QueryImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, provider string, region string) ([]subscriptions.Image, error) {
	testLock.Lock()
	defer testLock.Unlock()

	return testImages, nil
}

func (db *mockDBClient) QueryLatestImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, provider string, region string) (subscriptions.Image, error) {
	testLock.Lock()
	defer testLock.Unlock()

	return testImages[0], nil
}

func (db *mockDBClient) InsertImages(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Image) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()
	testImages = append(testImages, insertParams...)

	return len(testImages), nil
}

func (db *mockDBClient) UpdateImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams subscriptions.Image) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	for index, testImage := range testImages {
		if testImage.Region == updateParams.Region &&
			testImage.Provider == updateParams.Provider {
			testImages[index] = updateParams
		}
	}
	return len(testImages), nil
}

func (db *mockDBClient) InsertMandelboxes(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Mandelbox) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()
	testMandelboxes = append(testMandelboxes, insertParams...)

	return len(testMandelboxes), nil
}
func (db *mockDBClient) QueryMandelbox(context.Context, subscriptions.WhistGraphQLClient, string, string) ([]subscriptions.Mandelbox, error) {
	return testMandelboxes, nil
}

func (db *mockDBClient) UpdateMandelbox(context.Context, subscriptions.WhistGraphQLClient, subscriptions.Mandelbox) (int, error) {
	return 0, nil
}

// mockHostHandler is used to test all interactions with cloud providers
type mockHostHandler struct{}

func (mh *mockHostHandler) Initialize(region string) error {
	return nil
}

func (mh *mockHostHandler) SpinUpInstances(scalingCtx context.Context, numInstances int32, maxWaitTime time.Duration, image subscriptions.Image) (createdInstances []subscriptions.Instance, err error) {
	var newInstances []subscriptions.Instance
	for i := 0; i < int(numInstances); i++ {
		newInstances = append(newInstances, subscriptions.Instance{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           image.ImageID,
			ClientSHA:         image.ClientSHA,
			Type:              "g4dn.2xlarge",
			RemainingCapacity: instanceCapacity["g4dn.2xlarge"],
			Status:            "PRE_CONNECTION",
		})
	}

	return newInstances, nil
}

func (mh *mockHostHandler) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) error {
	return nil
}

func (mh *mockHostHandler) WaitForInstanceTermination(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	return nil
}

func (mh *mockHostHandler) WaitForInstanceReady(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	return nil
}

// mockHostHandler is used to test all interactions with hasura

type mockGraphQLClient struct{}

func (mg *mockGraphQLClient) Initialize(bool) error {
	return nil
}

func (mg *mockGraphQLClient) Query(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

func (mg *mockGraphQLClient) Mutate(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

func setup() {
	// Create and initialize a test scaling algorithm
	testAlgorithm = &DefaultScalingAlgorithm{
		Region: "test-region",
		Host:   &mockHostHandler{},
	}
	testDBClient := &mockDBClient{}
	testGraphQLClient := &mockGraphQLClient{}
	testAlgorithm.CreateEventChans()
	testAlgorithm.CreateGraphQLClient(testGraphQLClient)
	testAlgorithm.CreateDBClient(testDBClient)
	testLock = &sync.Mutex{}

	// Set the desired mandelboxes map to a default value for testing
	desiredFreeMandelboxesPerRegion = map[string]int{
		"test-region": 2,
	}

	config.Initialize(context.TODO(), testGraphQLClient)
}

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	os.Exit(code)
}
