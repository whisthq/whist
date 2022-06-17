// This file defines mock types and methods to test the
// different actions on the scaling algorithm.

package scaling_algorithms

import (
	"context"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var (
	testInstances   subscriptions.WhistInstances
	testImages      subscriptions.WhistImages
	testMandelboxes subscriptions.WhistMandelboxes
	testAlgorithm   *DefaultScalingAlgorithm
	testLock        sync.Mutex
)

// mockDBClient is used to test all database interactions
type mockDBClient struct{}

func (db *mockDBClient) QueryInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (subscriptions.WhistInstances, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var result subscriptions.WhistInstances
	for _, instance := range testInstances {
		if string(instance.ID) == instanceID {
			result = append(result, instance)
		}
	}

	return result, nil
}

func (db *mockDBClient) QueryInstanceWithCapacity(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, region string) (subscriptions.WhistInstances, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var instancesWithCapacity subscriptions.WhistInstances
	for _, instance := range testInstances {
		if string(instance.Region) == region && instance.RemainingCapacity > 0 {
			instancesWithCapacity = append(instancesWithCapacity, struct {
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
				ClientSHA:         graphql.String(instance.ClientSHA),
				Region:            graphql.String(instance.Region),
				Type:              graphql.String(instance.Type),
				RemainingCapacity: graphql.Int(instance.RemainingCapacity),
				IPAddress:         instance.IPAddress,
				Status:            graphql.String(instance.Status),
			})
		}
	}
	return instancesWithCapacity, nil
}

func (db *mockDBClient) QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, status string, region string) (subscriptions.WhistInstances, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var result subscriptions.WhistInstances
	for _, instance := range testInstances {
		if string(instance.Status) == status &&
			string(instance.Region) == region {
			result = append(result, instance)
		}
	}

	return result, nil
}

func (db *mockDBClient) QueryInstancesByImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, imageID string) (subscriptions.WhistInstances, error) {
	testLock.Lock()
	defer testLock.Unlock()

	return testInstances, nil
}

func (db *mockDBClient) InsertInstances(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Instance) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

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
			ClientSHA:         graphql.String(instance.ClientSHA),
			Region:            graphql.String(instance.Region),
			Type:              graphql.String(instance.Type),
			IPAddress:         instance.IPAddress,
			RemainingCapacity: graphql.Int(instance.RemainingCapacity),
			Status:            graphql.String(instance.Status),
		})
	}

	return len(insertParams), nil
}

func (db *mockDBClient) UpdateInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams subscriptions.Instance) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	var updated int
	for index, instance := range testInstances {
		if string(instance.ID) == updateParams.ID {
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
				ID:                graphql.String(updateParams.ID),
				Provider:          instance.Provider,
				ImageID:           instance.ImageID,
				ClientSHA:         graphql.String(instance.ClientSHA),
				Region:            graphql.String(instance.Region),
				Type:              instance.Type,
				IPAddress:         instance.IPAddress,
				Status:            graphql.String(updateParams.Status),
				RemainingCapacity: instance.RemainingCapacity,
			}
			testInstances[index] = updatedInstance
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

func (db *mockDBClient) QueryImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, provider string, region string) (subscriptions.WhistImages, error) {
	testLock.Lock()
	defer testLock.Unlock()

	return testImages, nil
}

func (db *mockDBClient) InsertImages(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Image) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	for _, image := range insertParams {
		testImages = append(testImages, struct {
			Provider  graphql.String `graphql:"provider"`
			Region    graphql.String `graphql:"region"`
			ImageID   graphql.String `graphql:"image_id"`
			ClientSHA graphql.String `graphql:"client_sha"`
			UpdatedAt time.Time      `graphql:"updated_at"`
		}{
			Provider:  graphql.String(image.Provider),
			Region:    graphql.String(image.Region),
			ImageID:   graphql.String(image.ImageID),
			ClientSHA: graphql.String(image.ClientSHA),
			UpdatedAt: image.UpdatedAt,
		})
	}
	return len(testImages), nil
}

func (db *mockDBClient) UpdateImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, image subscriptions.Image) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	for index, testImage := range testImages {
		if (testImage.Region == graphql.String(image.Region)) &&
			testImage.Provider == graphql.String(image.Provider) {
			updatedImage := struct {
				Provider  graphql.String `graphql:"provider"`
				Region    graphql.String `graphql:"region"`
				ImageID   graphql.String `graphql:"image_id"`
				ClientSHA graphql.String `graphql:"client_sha"`
				UpdatedAt time.Time      `graphql:"updated_at"`
			}{
				Provider:  graphql.String(image.Provider),
				Region:    graphql.String(image.Region),
				ImageID:   graphql.String(image.ImageID),
				ClientSHA: graphql.String(image.ClientSHA),
				UpdatedAt: image.UpdatedAt,
			}
			testImages[index] = updatedImage
		}
	}
	return len(testImages), nil
}

func (db *mockDBClient) InsertMandelboxes(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Mandelbox) (int, error) {
	testLock.Lock()
	defer testLock.Unlock()

	for _, mandelbox := range insertParams {
		testMandelboxes = append(testMandelboxes, struct {
			ID         graphql.String `graphql:"id"`
			App        graphql.String `graphql:"app"`
			InstanceID graphql.String `graphql:"instance_id"`
			UserID     graphql.String `graphql:"user_id"`
			SessionID  graphql.String `graphql:"session_id"`
			Status     graphql.String `graphql:"status"`
			CreatedAt  time.Time      `graphql:"created_at"`
			UpdatedAt  time.Time      `graphql:"updated_at"`
		}{
			ID:         graphql.String(mandelbox.ID.String()),
			App:        graphql.String(mandelbox.App),
			InstanceID: graphql.String(mandelbox.InstanceID),
			UserID:     graphql.String(mandelbox.UserID),
			SessionID:  graphql.String(mandelbox.SessionID),
			Status:     graphql.String(mandelbox.Status),
			CreatedAt:  mandelbox.CreatedAt,
			UpdatedAt:  mandelbox.UpdatedAt,
		})
	}
	return len(testMandelboxes), nil
}
func (db *mockDBClient) QueryMandelbox(context.Context, subscriptions.WhistGraphQLClient, string, string) (subscriptions.WhistMandelboxes, error) {
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
			ID:        "test-scale-up-instance",
			Provider:  "AWS",
			ImageID:   image.ImageID,
			ClientSHA: image.ClientSHA,
			Type:      "g4dn.2xlarge",
			Status:    "PRE_CONNECTION",
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

	// Set the desired mandelboxes map to a default value for testing
	desiredFreeMandelboxesPerRegion = map[string]int{
		"test-region": 2,
	}
}

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	os.Exit(code)
}
