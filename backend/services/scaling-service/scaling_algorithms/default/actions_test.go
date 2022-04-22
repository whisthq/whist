package scaling_algorithms

import (
	"context"
	"os"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/google/uuid"
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
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

	return testInstances, nil
}

func (db *mockDBClient) QueryInstanceWithCapacity(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, region string) (subscriptions.WhistInstances, error) {
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

	return testInstances, nil
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

	testInstances = subscriptions.WhistInstances{}
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
		}{
			ID:         graphql.String(mandelbox.ID.String()),
			App:        graphql.String(mandelbox.App),
			InstanceID: graphql.String(mandelbox.InstanceID),
			UserID:     graphql.String(mandelbox.UserID),
			SessionID:  graphql.String(mandelbox.SessionID),
			Status:     graphql.String(mandelbox.Status),
			CreatedAt:  mandelbox.CreatedAt,
		})
	}
	return len(testMandelboxes), nil
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
	err := testAlgorithm.VerifyInstanceScaleDown(context, ScalingEvent{Region: "test-region"}, subscriptions.Instance{
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
	err := testAlgorithm.VerifyCapacity(context, ScalingEvent{Region: "test-region"})
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
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
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
	err := testAlgorithm.ScaleDownIfNecessary(context, ScalingEvent{Region: "test-region"})
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
	err := testAlgorithm.ScaleUpIfNecessary(testInstancesToScale, context, ScalingEvent{Region: "test-region"}, subscriptions.Image{
		ImageID: "test-image-id-scale-up",
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

func TestDeploy(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
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
			ImageID:   "test-image-id-old",
			ClientSHA: "test-sha-dev",
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	testVersion := subscriptions.ClientAppVersion{
		DevCommitHash:     metadata.GetGitCommit(),
		StagingCommitHash: metadata.GetGitCommit(),
		ProdCommitHash:    metadata.GetGitCommit(),
	}

	wg := &sync.WaitGroup{}

	wg.Add(1)
	go func() {
		defer wg.Done()

		// For this test, start an image upgrade, check that instances with the new image
		// are created and that old instances are left untouched.
		err := testAlgorithm.UpgradeImage(context, ScalingEvent{Region: "test-region"}, "test-image-id-new")
		if err != nil {
			t.Errorf("Failed while testing scale up action. Err: %v", err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		err := testAlgorithm.SwapOverImages(context, ScalingEvent{Region: "test-region"}, testVersion)
		if err != nil {
			t.Errorf("Failed to swapover images. Err: %v", err)
		}
	}()

	wg.Wait()

	// Check that an instance was scaled up after the test instance was removed
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
			ClientSHA:         "test-sha-dev",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-new",
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

	// Check that the new image was unprotected successfully
	ok = compareProtectedMaps(testAlgorithm.protectedFromScaleDown, map[string]subscriptions.Image{})
	if !ok {
		t.Errorf("Expected protected from scale down map to be empty, got: %v", testAlgorithm.protectedFromScaleDown)
	}

	expectedImages := subscriptions.WhistImages{
		struct {
			Provider  graphql.String `graphql:"provider"`
			Region    graphql.String `graphql:"region"`
			ImageID   graphql.String `graphql:"image_id"`
			ClientSHA graphql.String `graphql:"client_sha"`
			UpdatedAt time.Time      `graphql:"updated_at"`
		}{
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id-new",
			ClientSHA: graphql.String(metadata.GetGitCommit()),
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	ok = compareWhistImages(testImages, expectedImages)
	if !ok {
		t.Errorf("Swapover did not insert the correct images to database. Expected %v, got %v", expectedImages, testImages)
	}
}

// TestMandelboxAssign test the happy path of assigning a mandelbox to a user.
func TestMandelboxAssign(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	var tests = []struct {
		name             string
		capacity         int
		clientSHA, want  string
		shouldBeAssigned bool
	}{
		{"happy path", instanceCapacity["g4dn.2xlarge"], CLIENT_COMMIT_HASH_DEV_OVERRIDE, "", true},               // Happy path, sufficient capacity and matching commit hash
		{"commit hash mismatch", instanceCapacity["g4dn.2xlarge"], "outdated-sha", "COMMIT_HASH_MISMATCH", false}, // Commit mismatch, sufficient capacity but different commit hashes
		{"no capacity", 0, CLIENT_COMMIT_HASH_DEV_OVERRIDE, "NO_INSTANCE_AVAILABLE", false},                       // No capacity, but matching commit hash
	}

	// Override environment so we can test commit hashes on the request
	metadata.GetAppEnvironment = func() metadata.AppEnvironment {
		return metadata.EnvDev
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Populate test instances that will be used when
			// mocking database functions.
			testInstances = subscriptions.WhistInstances{
				{
					ID:                "test-assign-instance-1",
					Provider:          "AWS",
					ImageID:           "test-image-id",
					Status:            "ACTIVE",
					Type:              "g4dn.2xlarge",
					Region:            "us-east-1",
					IPAddress:         "1.1.1.1/24",
					ClientSHA:         graphql.String("test-sha"),
					RemainingCapacity: graphql.Int(tt.capacity),
				},
				{
					ID:                "test-assign-instance-2",
					Provider:          "AWS",
					ImageID:           "test-image-id",
					Status:            "ACTIVE",
					Type:              "g4dn.2xlarge",
					Region:            "us-west-1",
					IPAddress:         "1.1.1.1/24",
					ClientSHA:         graphql.String("test-sha"),
					RemainingCapacity: graphql.Int(tt.capacity),
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
					ImageID:   "test-image-id-old",
					ClientSHA: "test-sha",
					UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
				},
			}

			testAssignRequest := &httputils.MandelboxAssignRequest{
				Regions:    []string{"us-east-1", "us-west-1"},
				CommitHash: tt.clientSHA,
				SessionID:  1234567890,
				UserEmail:  "user@whist.com",
			}
			testAssignRequest.CreateResultChan()

			wg := &sync.WaitGroup{}
			errorChan := make(chan error, 1)

			wg.Add(1)
			go func() {
				defer wg.Done()

				err := testAlgorithm.MandelboxAssign(context, ScalingEvent{Data: testAssignRequest})
				errorChan <- err
			}()

			var assignResult httputils.MandelboxAssignRequestResult
			wg.Add(1)
			go func() {
				defer wg.Done()

				res := <-testAssignRequest.ResultChan
				assignResult = res.Result.(httputils.MandelboxAssignRequestResult)
			}()

			wg.Wait()

			// Print the errors from the assign action to verify the
			// behavior is the expected one.
			err := <-errorChan
			t.Logf("Error while testing mandelbox assign. Err: %v", err)

			if assignResult.Error != tt.want {
				t.Errorf("Expected mandelbox assign request Error field to be %v, got %v", tt.want, assignResult.Error)
			}

			id, err := uuid.Parse(assignResult.MandelboxID.String())
			if err != nil {
				t.Errorf("Got an invalid Mandelbox ID from the assign request %v. Err: %v", id, err)
			}

			if tt.shouldBeAssigned && assignResult.IP != "1.1.1.1" {
				t.Errorf("None of the test instances were assigned correctly.")
			}
		})
	}

}

// Helper functions

// We need to use a custom compare function to compare protectedFromScaleDown maps
// because the `UpdatedAt` is a timestamp set with `time.Now`.
func compareProtectedMaps(a map[string]subscriptions.Image, b map[string]subscriptions.Image) bool {
	var equal bool

	if (len(a) == 0) && (len(b) == 0) {
		equal = true
	}

	for k, v := range a {
		for s, i := range b {
			if k != s {
				break
			}
			i.UpdatedAt = v.UpdatedAt
			equal = reflect.DeepEqual(v, i)
		}
	}

	return equal
}

// We need to use a custom compare function to compare WhistImages objects
// because the `UpdatedAt` is a timestamp set with `time.Now`.
func compareWhistImages(a subscriptions.WhistImages, b subscriptions.WhistImages) bool {
	var equal bool
	for _, v := range a {
		for _, i := range b {
			i.UpdatedAt = v.UpdatedAt
			equal = reflect.DeepEqual(v, i)
		}
	}

	return equal
}
