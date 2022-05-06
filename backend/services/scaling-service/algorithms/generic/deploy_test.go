package generic

import (
	"context"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

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
		err := testAlgorithm.UpgradeImage(context, algorithms.ScalingEvent{Region: "test-region"}, "test-image-id-new")
		if err != nil {
			t.Errorf("Failed while testing scale up action. Err: %v", err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		err := testAlgorithm.SwapOverImages(context, algorithms.ScalingEvent{Region: "test-region"}, testVersion)
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
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	ok = compareWhistImages(testImages, expectedImages)
	if !ok {
		t.Errorf("Swapover did not insert the correct images to database. Expected %v, got %v", expectedImages, testImages)
	}
}
