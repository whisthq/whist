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

var testImages []subscriptions.WhistImage

type mockImagesGraphQLClient struct{}

func (mc *mockImagesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockImagesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		WhistImages []subscriptions.WhistImage `graphql:"whist_images(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
	}:
		query.WhistImages = testImages
	default:
	}

	return nil
}

func (mc *mockImagesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_images(objects: $objects)"`
	}:
		for _, image := range vars["objects"].([]whist_images_insert_input) {
			testImages = append(testImages, subscriptions.WhistImage{
				Provider:  image.Provider,
				Region:    image.Region,
				ImageID:   image.ImageID,
				ClientSHA: image.ClientSHA,
				UpdatedAt: image.UpdatedAt,
			})
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testImages))

	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_images(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, image_id: $image_id, provider: $provider, region: $region, updated_at: $updated_at})"`
	}:
		for i := 0; i < len(testImages); i++ {
			if testImages[i].ImageID == vars["image_id"] {
				testImages[i].ImageID = vars["image_id"].(graphql.String)
				testImages[i].Region = vars["region"].(graphql.String)
				testImages[i].Provider = vars["provider"].(graphql.String)
				testImages[i].ClientSHA = vars["client_sha"].(graphql.String)
				testImages[i].UpdatedAt = vars["updated_at"].(timestamptz).Time
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testImages))

	default:
	}
	return nil
}

func TestQueryImage(t *testing.T) {
	mockClient := &mockImagesGraphQLClient{}
	testDBClient := &DBClient{}

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

			res, err := testDBClient.QueryImage(context.Background(), mockClient, string(tt.expected.Provider), string(tt.expected.Region))
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
	mockClient := &mockImagesGraphQLClient{}
	testDBClient := &DBClient{}

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

			rows, err := testDBClient.InsertImages(context.Background(), mockClient, subscriptions.ToImages(tt.expected))
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
	mockClient := &mockImagesGraphQLClient{}
	testDBClient := &DBClient{}

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

			rows, err := testDBClient.UpdateImage(context.Background(), mockClient, expected[0])
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
