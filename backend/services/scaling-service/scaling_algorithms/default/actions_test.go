package scaling_algorithms

import (
	"context"
	"os"
	"reflect"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var (
	testInstances subscriptions.WhistInstances
	testImages    subscriptions.WhistImages
	testAlgorithm *DefaultScalingAlgorithm
)

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
	return testImages, nil
}

func (db *mockDBClient) InsertImages(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Image) (int, error) {
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

// mockHostHandler is used to test all interactions with cloud providers
type mockHostHandler struct{}

func (mh *mockHostHandler) Initialize(region string) error {
	return nil
}

func (mh *mockHostHandler) SpinUpInstances(scalingCtx context.Context, numInstances int32, maxWaitTime time.Duration, imageID string) (createdInstances []subscriptions.Instance, err error) {
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
			Status:            "DRAINING",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-down-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "DRAINING",
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
	err := testAlgorithm.ScaleUpIfNecessary(testInstancesToScale, context, ScalingEvent{Region: "test-region"}, "test-image-id-scale-up")
	if err != nil {
		t.Errorf("Failed while testing scale up action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-scale-up",
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

func TestUpgradeImage(t *testing.T) {
	t.Parallel()
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
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
			ClientSHA: graphql.String(metadata.GetGitCommit()),
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	// For this test, start an image upgrade, check that instances with the new image
	// are created and that old instances are left untouched.
	err := testAlgorithm.UpgradeImage(context, ScalingEvent{Region: "test-region"}, "test-image-id-new")
	if err != nil {
		t.Errorf("Failed while testing scale up action. Err: %v", err)
	}

	// Check that an instance was scaled up after the test instance was removed
	expectedInstances := subscriptions.WhistInstances{
		{
			ID:                "test-image-upgrade-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-old",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
		{
			ID:                "test-scale-up-instance",
			Provider:          "AWS",
			ImageID:           "test-image-id-new",
			Status:            "PRE_CONNECTION",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: graphql.Int(instanceCapacity["g4dn.2xlarge"]),
		},
	}
	ok := reflect.DeepEqual(testInstances, expectedInstances)
	if !ok {
		t.Errorf("Did not scale up instances correctly while testing scale up action. Expected %v, got %v", expectedInstances, testInstances)
	}

	testProtectedMap := map[string]subscriptions.Image{
		"test-image-id-new": {
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id-new",
			ClientSHA: metadata.GetGitCommit(),
		},
	}
	// Check that the new image was protected from scale down
	ok = compareProtectedMaps(testAlgorithm.protectedFromScaleDown, testProtectedMap)
	if !ok {
		t.Errorf("Protected from scale down map was not set correctly. Expected %v, got %v.", testProtectedMap, testAlgorithm.protectedFromScaleDown)
	}
}

func TestSwapOverImages(t *testing.T) {
	t.Parallel()
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	testVersion := subscriptions.ClientAppVersion{
		DevCommitHash:     metadata.GetGitCommit(),
		StagingCommitHash: metadata.GetGitCommit(),
		ProdCommitHash:    metadata.GetGitCommit(),
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
			ImageID:   "test-swapover-image-id",
			ClientSHA: graphql.String(metadata.GetGitCommit()),
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	err := testAlgorithm.SwapOverImages(context, ScalingEvent{Region: "test-region"}, testVersion)
	if err != nil {
		t.Errorf("Failed to swapover images. Err: %v", err)
	}

	// Check that the new image was unprotected successfully
	ok := compareProtectedMaps(testAlgorithm.protectedFromScaleDown, map[string]subscriptions.Image{})
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
