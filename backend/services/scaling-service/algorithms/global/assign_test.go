package global

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/google/uuid"
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

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

				err := testAlgorithm.MandelboxAssign(context, algorithms.ScalingEvent{Data: testAssignRequest})
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
