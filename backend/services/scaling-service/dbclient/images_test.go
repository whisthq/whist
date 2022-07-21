package dbclient

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/google/uuid"
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestQueryImage(t *testing.T) {
	var tests = []struct {
		name     string
		expected subscriptions.WhistImage
	}{
		{"Successful query", subscriptions.WhistImage{Provider: "AWS", Region: "us-east-1", ImageID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()}},
		{"Not found", subscriptions.WhistImage{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testImages = []subscriptions.WhistImage{tt.expected}

			res, err := testDBClient.QueryImage(context.Background(), mockImagesClient, string(tt.expected.Provider), string(tt.expected.Region))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 {
				t.Fatalf("failed to query for image, got empty result")
			}

			if ok := reflect.DeepEqual(res, subscriptions.ToImages([]subscriptions.WhistImage{tt.expected})); !ok {
				t.Fatalf("incorrect image returned from query, expected %v, got %v", tt.expected, res[0])
			}
		})
	}
}

func TestInsertImages(t *testing.T) {
	var tests = []struct {
		name     string
		expected []subscriptions.WhistImage
	}{
		{"Insert one", []subscriptions.WhistImage{
			{Provider: "AWS", Region: "us-east-1", ImageID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
		{"Insert multiple", []subscriptions.WhistImage{
			{Provider: "AWS", Region: "us-east-1", ImageID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
			{Provider: "GCloud", Region: "us-west1", ImageID: graphql.String(uuid.NewString()), ClientSHA: "test_sha_2", UpdatedAt: time.Now().Add(5 * time.Minute)},
		}},
		{"Insert zero", []subscriptions.WhistImage{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testImages = []subscriptions.WhistImage{}

			rows, err := testDBClient.InsertImages(context.Background(), mockImagesClient, subscriptions.ToImages(tt.expected))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.expected) {
				t.Fatalf("incorrect number of rows inserted, expected %d rows, inserted %d", len(tt.expected), rows)
			}

			if ok := reflect.DeepEqual(testImages, tt.expected); !ok {
				t.Fatalf("incorrect images inserted, expected %v, got %v", tt.expected, testImages)
			}
		})
	}
}

func TestUpdateImage(t *testing.T) {
	var tests = []struct {
		name     string
		expected []subscriptions.WhistImage
	}{
		{"Update existing", []subscriptions.WhistImage{
			{Provider: "AWS", Region: "us-east-1", ImageID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "new_test_sha", UpdatedAt: time.Now()}},
		},
		{"Update non existing", []subscriptions.WhistImage{
			{Provider: "Azure", Region: "az1", ImageID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testImages = []subscriptions.WhistImage{{Provider: "GCloud", Region: "us-east-2", ImageID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "old_test_sha", UpdatedAt: time.Now()}}
			expected := subscriptions.ToImages(tt.expected)

			rows, err := testDBClient.UpdateImage(context.Background(), mockImagesClient, expected[0])
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(expected) {
				t.Fatalf("incorrect number of rows updated, expected %d rows, inserted %d", len(expected), rows)
			}

			if ok := reflect.DeepEqual(testImages, tt.expected); !ok {
				t.Fatalf("incorrect images updated, expected %v, got %v", tt.expected, testImages)
			}
		})
	}
}
