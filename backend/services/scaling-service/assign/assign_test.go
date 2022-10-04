package assign

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/config"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

var (
	testInstances   []subscriptions.Instance
	testImages      []subscriptions.Image
	testMandelboxes []subscriptions.Mandelbox
)

var defaultRegions []string = []string{"us-east-1", "us-west-1"}

var envs = []metadata.AppEnvironment{
	metadata.EnvDev,
	metadata.EnvStaging,
	metadata.EnvProd,
}

// TestMandelboxAssign test the happy path of assigning a mandelbox to a user.
func TestMandelboxAssign(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	var tests = []struct {
		name             string
		capacity         int
		regions          []string
		clientSHA, want  string
		shouldBeAssigned bool
	}{
		{"happy path", 2, defaultRegions, CLIENT_COMMIT_HASH_DEV_OVERRIDE, "", true},                      // Happy path, sufficient capacity and matching commit hash
		{"commit hash mismatch", 2, defaultRegions, "outdated-sha", COMMIT_HASH_MISMATCH, false},          // Commit mismatch, sufficient capacity but different commit hashes
		{"no capacity", 0, defaultRegions, CLIENT_COMMIT_HASH_DEV_OVERRIDE, NO_INSTANCE_AVAILABLE, false}, // No capacity, but matching commit hash
		{"some unavailable regions", 2, []string{
			"unavailable-region-1",
			"us-west-1",
			"unavailable-region-2",
			"us-east-1",
		}, CLIENT_COMMIT_HASH_DEV_OVERRIDE, "", true}, // Some unavailable regions
		{"only unavailable regions", 2, []string{
			"unavailable-region-1",
			"unavailable-region-2",
			"unavailable-region-3",
			"unavailable-region-4",
		}, CLIENT_COMMIT_HASH_DEV_OVERRIDE, REGION_NOT_ENABLED, false}, // Only unavailable regions
	}

	for _, env := range envs {
		for _, tt := range tests {
			t.Run(tt.name+"_"+string(env), func(t *testing.T) {
				// Override environment so we can test commit hashes on the request
				metadata.GetAppEnvironment = func() metadata.AppEnvironment {
					return env
				}

				// Populate test instances that will be used when
				// mocking database functions.
				testInstances = []subscriptions.Instance{
					{
						ID:                "test-assign-instance-1",
						Provider:          "AWS",
						ImageID:           "test-image-id",
						Status:            "ACTIVE",
						Type:              "g4dn.2xlarge",
						Region:            "us-east-1",
						IPAddress:         "1.1.1.1/24",
						ClientSHA:         "test-sha",
						RemainingCapacity: tt.capacity,
					},
					{
						ID:                "test-assign-instance-2",
						Provider:          "AWS",
						ImageID:           "test-image-id",
						Status:            "ACTIVE",
						Type:              "g4dn.2xlarge",
						Region:            "us-west-1",
						IPAddress:         "1.1.1.1/24",
						ClientSHA:         "test-sha",
						RemainingCapacity: tt.capacity,
					},
					{
						ID:                "test-assign-instance-3",
						Provider:          "AWS",
						ImageID:           "test-image-id",
						Status:            "ACTIVE",
						Type:              "g4dn.2xlarge",
						Region:            "ap-south-1",
						IPAddress:         "1.1.1.1/24",
						ClientSHA:         "test-sha",
						RemainingCapacity: tt.capacity,
					},
				}

				// Set the current image for testing
				testImages = []subscriptions.Image{
					{
						Provider:  "AWS",
						Region:    "test-region",
						ImageID:   "test-image-id",
						ClientSHA: "test-sha",
						UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
					},
				}

				testMandelboxes = []subscriptions.Mandelbox{
					{
						ID:         types.MandelboxID(uuid.New()),
						App:        "CHROME",
						InstanceID: "test-assign-instance-1",
						SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
						Status:     "WAITING",
						CreatedAt:  time.Now(),
						UpdatedAt:  time.Now(),
					},
					{
						ID:         types.MandelboxID(uuid.New()),
						App:        "CHROME",
						InstanceID: "test-assign-instance-2",
						SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
						Status:     "WAITING",
						CreatedAt:  time.Now(),
						UpdatedAt:  time.Now(),
					},
					{
						ID:         types.MandelboxID(uuid.New()),
						App:        "CHROME",
						InstanceID: "test-assign-instance-3",
						SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
						Status:     "WAITING",
						CreatedAt:  time.Now(),
						UpdatedAt:  time.Now(),
					},
				}

				testAssignRequest := &httputils.MandelboxAssignRequest{
					Regions:    tt.regions,
					CommitHash: tt.clientSHA,
					UserEmail:  "user@whist.com",
					UserID:     types.UserID(uuid.NewString()),
					Version:    "2.13.2",
				}
				testAssignRequest.CreateResultChan()

				wg := &sync.WaitGroup{}
				errorChan := make(chan error, 1)

				frontendVersion := subscriptions.FrontendVersion{
					ID:    1,
					Major: 3,
					Minor: 0,
					Micro: 0,
				}
				config.SetFrontendVersion(frontendVersion)

				wg.Add(1)
				go func() {
					defer wg.Done()

					err := MandelboxAssign(context, testAssignRequest, defaultRegions[0])
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
				t.Log(err)
				if err != nil && tt.shouldBeAssigned {
					t.Errorf("error while testing mandelbox assign: %s", err)
				}

				if assignResult.Error != tt.want {
					t.Errorf("expected mandelbox assign request Error field to be %v, got %v", tt.want, assignResult.Error)
				}

				id, err := uuid.Parse(assignResult.MandelboxID.String())
				if err != nil {
					t.Errorf("got an invalid Mandelbox ID from the assign request %s: %s", id, err)
				}

				if tt.shouldBeAssigned && assignResult.IP != "1.1.1.1" {
					t.Errorf("none of the test instances were assigned correctly.")
				}
			})
		}
	}
}

// TestMandelboxLimit will try to request more mandelboxes than the set limit
// and verify the correct response is sent when exceeding the limit.
func TestMandelboxLimit(t *testing.T) {
	context, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Populate test instances that will be used when
	// mocking database functions.
	testInstances = []subscriptions.Instance{
		{
			ID:                "test-assign-instance-1",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			Region:            "us-east-1",
			IPAddress:         "1.1.1.1/24",
			ClientSHA:         "test-sha",
			RemainingCapacity: 1,
		},
		{
			ID:                "test-assign-instance-2",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			Region:            "us-east-1",
			IPAddress:         "1.1.1.1/24",
			ClientSHA:         "test-sha",
			RemainingCapacity: 1,
		},
		{
			ID:                "test-assign-instance-3",
			Provider:          "AWS",
			ImageID:           "test-image-id",
			Status:            "ACTIVE",
			Type:              "g4dn.2xlarge",
			Region:            "us-east-1",
			IPAddress:         "1.1.1.1/24",
			ClientSHA:         "test-sha",
			RemainingCapacity: 1,
		},
	}

	// Set the current image for testing
	testImages = []subscriptions.Image{
		{
			Provider:  "AWS",
			Region:    "test-region",
			ImageID:   "test-image-id",
			ClientSHA: "test-sha",
			UpdatedAt: time.Date(2022, 04, 11, 11, 54, 30, 0, time.Local),
		},
	}

	testMandelboxes = []subscriptions.Mandelbox{
		{
			ID:         types.MandelboxID(uuid.New()),
			App:        "CHROME",
			InstanceID: "test-assign-instance-1",
			SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
			Status:     "WAITING",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		},
		{
			ID:         types.MandelboxID(uuid.New()),
			App:        "CHROME",
			InstanceID: "test-assign-instance-2",
			SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
			Status:     "WAITING",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		},
		{
			ID:         types.MandelboxID(uuid.New()),
			App:        "CHROME",
			InstanceID: "test-assign-instance-3",
			SessionID:  utils.Sprintf("%v", time.Now().UnixMilli()),
			Status:     "WAITING",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		},
	}

	testAssignRequest := &httputils.MandelboxAssignRequest{
		Regions:    []string{"us-east-1"},
		CommitHash: "test-sha",
		UserEmail:  "user@whist.com",
		UserID:     "test_user_id",
		Version:    "2.13.2",
	}
	testAssignRequest.CreateResultChan()

	frontendVersion := subscriptions.FrontendVersion{
		ID:    1,
		Major: 3,
		Minor: 0,
		Micro: 0,
	}
	config.SetFrontendVersion(frontendVersion)

	wg := &sync.WaitGroup{}
	errorChan := make(chan error, 1)

	for i := 0; i <= int(config.GetMandelboxLimitPerUser()); i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := MandelboxAssign(context, testAssignRequest, defaultRegions[0])
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
		err := <-errorChan

		if err != nil &&
			i < int(config.GetMandelboxLimitPerUser()) {
			t.Errorf("did not expect error, got: %s", err)
		}

		if assignResult.Error == "USER_ALREADY_ACTIVE" &&
			i < int(config.GetMandelboxLimitPerUser()) {
			t.Errorf("request got limited when not exceeding limit")
		}

		if assignResult.Error == "USER_ALREADY_ACTIVE" &&
			i == int(config.GetMandelboxLimitPerUser()) {
			t.Log(assignResult)
			t.Log("Request got limited correctly after exceeding mandelbox limit")
		}
	}

}
