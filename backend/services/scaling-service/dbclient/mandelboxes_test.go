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

var testMandelboxes []subscriptions.WhistMandelbox

type mockMandelboxesGraphQLClient struct{}

func (mc *mockMandelboxesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockMandelboxesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		WhistMandelboxes []subscriptions.WhistMandelbox `graphql:"whist_mandelboxes(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
	}:
		query.WhistMandelboxes = testMandelboxes
	default:
	}

	return nil
}

func (mc *mockMandelboxesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_mandelboxes(objects: $objects)"`
	}:
		for _, mandelbox := range vars["objects"].([]whist_mandelboxes_insert_input) {
			testMandelboxes = append(testMandelboxes, subscriptions.WhistMandelbox{
				Provider:  mandelbox.Provider,
				Region:    mandelbox.Region,
				ID:        mandelbox.ID,
				ClientSHA: mandelbox.ClientSHA,
				UpdatedAt: mandelbox.UpdatedAt,
			})
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testMandelboxes))

	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_mandelboxes(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, mandelbox_id: $mandelbox_id, provider: $provider, region: $region, updated_at: $updated_at})"`
	}:
		for i := 0; i < len(testMandelboxes); i++ {
			if testMandelboxes[i].ID == vars["mandelbox_id"] {
				testMandelboxes[i].ID = vars["mandelbox_id"].(graphql.String)
				testMandelboxes[i].Region = vars["region"].(graphql.String)
				testMandelboxes[i].Provider = vars["provider"].(graphql.String)
				testMandelboxes[i].ClientSHA = vars["client_sha"].(graphql.String)
				testMandelboxes[i].UpdatedAt = vars["updated_at"].(timestamptz).Time
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testMandelboxes))

	default:
	}
	return nil
}

func TestQueryMandelbox(t *testing.T) {
	mockClient := &mockMandelboxesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected subscriptions.WhistMandelbox
	}{
		{"Successful query", subscriptions.WhistMandelbox{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()}},
		{"Not found", subscriptions.WhistMandelbox{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMandelboxes = []subscriptions.WhistMandelbox{tt.expected}

			res, err := testDBClient.QueryMandelbox(context.Background(), mockClient, string(tt.expected.Provider), string(tt.expected.Region))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 {
				t.Fatalf("failed to query for mandelbox, got empty result")
			}

			if ok := reflect.DeepEqual(res, subscriptions.ToMandelboxes([]subscriptions.WhistMandelbox{tt.expected})); !ok {
				t.Fatalf("incorrect mandelbox returned from query, expected %v, got %v", tt.expected, res[0])
			}
		})
	}
}

func TestInsertMandelboxes(t *testing.T) {
	mockClient := &mockMandelboxesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected []subscriptions.WhistMandelbox
	}{
		{"Insert one", []subscriptions.WhistMandelbox{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
		{"Insert multiple", []subscriptions.WhistMandelbox{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
			{Provider: "GCloud", Region: "us-west1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha_2", UpdatedAt: time.Now().Add(5 * time.Minute)},
		}},
		{"Insert zero", []subscriptions.WhistMandelbox{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMandelboxes = []subscriptions.WhistMandelbox{}

			rows, err := testDBClient.InsertMandelboxes(context.Background(), mockClient, subscriptions.ToMandelboxes(tt.expected))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.expected) {
				t.Fatalf("incorrect number of rows inserted, expected %d rows, inserted %d", len(tt.expected), rows)
			}

			if ok := reflect.DeepEqual(testMandelboxes, tt.expected); !ok {
				t.Fatalf("incorrect mandelboxes inserted, expected %v, got %v", tt.expected, testMandelboxes)
			}
		})
	}
}

func TestUpdateMandelbox(t *testing.T) {
	mockClient := &mockMandelboxesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected []subscriptions.WhistMandelbox
	}{
		{"Update existing", []subscriptions.WhistMandelbox{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "new_test_sha", UpdatedAt: time.Now()}},
		},
		{"Update non existing", []subscriptions.WhistMandelbox{
			{Provider: "Azure", Region: "az1", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testMandelboxes = []subscriptions.WhistMandelbox{{Provider: "GCloud", Region: "us-east-2", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "old_test_sha", UpdatedAt: time.Now()}}
			expected := subscriptions.ToMandelboxes(tt.expected)

			rows, err := testDBClient.UpdateMandelbox(context.Background(), mockClient, expected[0])
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(expected) {
				t.Fatalf("incorrect number of rows updated, expected %d rows, inserted %d", len(expected), rows)
			}

			if ok := reflect.DeepEqual(testMandelboxes, tt.expected); !ok {
				t.Fatalf("incorrect mandelboxes updated, expected %v, got %v", tt.expected, testMandelboxes)
			}
		})
	}
}
