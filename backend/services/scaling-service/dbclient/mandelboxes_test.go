package dbclient

import (
	"context"
	"reflect"
	"testing"

	"github.com/google/uuid"
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestQueryMandelbox(t *testing.T) {
	var tests = []struct {
		name               string
		instanceID, status string
		err                bool
		expected           []subscriptions.Mandelbox
	}{
		{"Successful query", utils.PlaceholderWarmupUUID().String(), "RUNNING", false, []subscriptions.Mandelbox{
			{
				ID:         types.MandelboxID(utils.PlaceholderTestUUID()),
				App:        "CHROME",
				InstanceID: utils.PlaceholderWarmupUUID().String(),
				UserID:     "test-user-id",
				SessionID:  "1234567890",
				Status:     "RUNNING",
			},
		}},
		{"Not found", utils.PlaceholderWarmupUUID().String(), "RUNNING", true, nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMandelboxes = []subscriptions.WhistMandelbox{
				{
					ID:         graphql.String(utils.PlaceholderTestUUID().String()),
					App:        graphql.String("CHROME"),
					InstanceID: graphql.String(utils.PlaceholderWarmupUUID().String()),
					UserID:     graphql.String("test-user-id"),
					SessionID:  graphql.String("1234567890"),
					Status:     graphql.String("RUNNING"),
				},
				{
					ID:         graphql.String(uuid.MustParse("9e185e51-d60f-4e27-b7c7-48e554eec5c2").String()),
					App:        graphql.String("CHROME"),
					InstanceID: graphql.String(utils.PlaceholderWarmupUUID().String()),
					Status:     graphql.String("WAITING"),
				},
			}

			res, err := testDBClient.QueryMandelbox(context.Background(), mockMandelboxesClient, tt.instanceID, tt.status)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 && !tt.err {
				t.Fatalf("failed to query for mandelbox, got empty result")
			} else if tt.err {
				return
			}

			if ok := reflect.DeepEqual(tt.expected, res); !ok {
				t.Fatalf("incorrect mandelbox returned from query, expected %v, got %v", tt.expected, res[0])
			}
		})
	}
}

func TestUpdateMandelbox(t *testing.T) {
	var tests = []struct {
		name         string
		updated      int
		updateParams subscriptions.Mandelbox
		expected     []subscriptions.WhistMandelbox
	}{
		{"Update existing", 1, subscriptions.Mandelbox{
			ID:         types.MandelboxID(utils.PlaceholderTestUUID()),
			App:        "CHROME",
			InstanceID: utils.PlaceholderWarmupUUID().String(),
			UserID:     "test-user-id",
			SessionID:  "1234567890",
			Status:     "RUNNING",
		}, []subscriptions.WhistMandelbox{
			{
				ID:         graphql.String(utils.PlaceholderTestUUID().String()),
				App:        graphql.String("CHROME"),
				InstanceID: graphql.String(utils.PlaceholderWarmupUUID().String()),
				UserID:     graphql.String("test-user-id"),
				SessionID:  graphql.String("1234567890"),
				Status:     graphql.String("RUNNING"),
			},
		}},
		{"Update non existing", 0, subscriptions.Mandelbox{
			ID:         types.MandelboxID(uuid.MustParse("458f8640-1d12-47c1-8712-ae8b7a3e13ce")),
			App:        "CHROME",
			InstanceID: utils.PlaceholderWarmupUUID().String(),
			UserID:     "test-user-id",
			SessionID:  "1234567890",
			Status:     "RUNNING",
		}, []subscriptions.WhistMandelbox{
			{
				ID:         graphql.String(utils.PlaceholderTestUUID().String()),
				App:        graphql.String("CHROME"),
				InstanceID: graphql.String(utils.PlaceholderWarmupUUID().String()),
				Status:     graphql.String("WAITING"),
			},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMandelboxes = []subscriptions.WhistMandelbox{
				{
					ID:         graphql.String(utils.PlaceholderTestUUID().String()),
					App:        graphql.String("CHROME"),
					InstanceID: graphql.String(utils.PlaceholderWarmupUUID().String()),
					Status:     graphql.String("WAITING"),
				},
			}
			rows, err := testDBClient.UpdateMandelbox(context.Background(), mockMandelboxesClient, tt.updateParams)
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != tt.updated {
				t.Fatalf("incorrect number of rows updated, expected %d rows, updated %d", tt.updated, rows)
			}

			if ok := reflect.DeepEqual(testMandelboxes, tt.expected); !ok {
				t.Fatalf("incorrect images updated, expected %v, got %v", tt.expected, testMandelboxes)
			}
		})
	}
}
