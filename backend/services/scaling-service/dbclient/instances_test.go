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

var testInstances []subscriptions.WhistInstance

type mockInstancesGraphQLClient struct{}

func (mc *mockInstancesGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockInstancesGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		WhistInstances []subscriptions.WhistInstance `graphql:"whist_instances(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
	}:
		query.WhistInstances = testInstances
	default:
	}

	return nil
}

func (mc *mockInstancesGraphQLClient) Mutate(ctx context.Context, mutation subscriptions.GraphQLQuery, vars map[string]interface{}) error {
	switch mutation := mutation.(type) {
	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_instances(objects: $objects)"`
	}:
		for _, instance := range vars["objects"].([]whist_instances_insert_input) {
			testInstances = append(testInstances, subscriptions.WhistInstance{
				Provider:  instance.Provider,
				Region:    instance.Region,
				ID:        instance.ID,
				ClientSHA: instance.ClientSHA,
				UpdatedAt: instance.UpdatedAt,
			})
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testInstances))

	case *struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_instances(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, instance_id: $instance_id, provider: $provider, region: $region, updated_at: $updated_at})"`
	}:
		for i := 0; i < len(testInstances); i++ {
			if testInstances[i].ID == vars["instance_id"] {
				testInstances[i].ID = vars["instance_id"].(graphql.String)
				testInstances[i].Region = vars["region"].(graphql.String)
				testInstances[i].Provider = vars["provider"].(graphql.String)
				testInstances[i].ClientSHA = vars["client_sha"].(graphql.String)
				testInstances[i].UpdatedAt = vars["updated_at"].(timestamptz).Time
			}
		}
		mutation.MutationResponse.AffectedRows = graphql.Int(len(testInstances))

	default:
	}
	return nil
}

func TestQueryInstance(t *testing.T) {
	mockClient := &mockInstancesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected subscriptions.WhistInstance
	}{
		{"Successful query", subscriptions.WhistInstance{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()}},
		{"Not found", subscriptions.WhistInstance{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances = []subscriptions.WhistInstance{tt.expected}

			res, err := testDBClient.QueryInstance(context.Background(), mockClient, string(tt.expected.Provider), string(tt.expected.Region))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if len(res) < 1 {
				t.Fatalf("failed to query for instance, got empty result")
			}

			if ok := reflect.DeepEqual(res, subscriptions.ToInstances([]subscriptions.WhistInstance{tt.expected})); !ok {
				t.Fatalf("incorrect instance returned from query, expected %v, got %v", tt.expected, res[0])
			}
		})
	}
}

func TestInsertInstances(t *testing.T) {
	mockClient := &mockInstancesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected []subscriptions.WhistInstance
	}{
		{"Insert one", []subscriptions.WhistInstance{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
		{"Insert multiple", []subscriptions.WhistInstance{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
			{Provider: "GCloud", Region: "us-west1", ID: graphql.String(uuid.NewString()), ClientSHA: "test_sha_2", UpdatedAt: time.Now().Add(5 * time.Minute)},
		}},
		{"Insert zero", []subscriptions.WhistInstance{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances = []subscriptions.WhistInstance{}

			rows, err := testDBClient.InsertInstances(context.Background(), mockClient, subscriptions.ToInstances(tt.expected))
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(tt.expected) {
				t.Fatalf("incorrect number of rows inserted, expected %d rows, inserted %d", len(tt.expected), rows)
			}

			if ok := reflect.DeepEqual(testInstances, tt.expected); !ok {
				t.Fatalf("incorrect instances inserted, expected %v, got %v", tt.expected, testInstances)
			}
		})
	}
}

func TestUpdateInstance(t *testing.T) {
	mockClient := &mockInstancesGraphQLClient{}
	testDBClient := &DBClient{}

	var tests = []struct {
		name     string
		expected []subscriptions.WhistInstance
	}{
		{"Update existing", []subscriptions.WhistInstance{
			{Provider: "AWS", Region: "us-east-1", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "new_test_sha", UpdatedAt: time.Now()}},
		},
		{"Update non existing", []subscriptions.WhistInstance{
			{Provider: "Azure", Region: "az1", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "test_sha", UpdatedAt: time.Now()},
		}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testInstances = []subscriptions.WhistInstance{{Provider: "GCloud", Region: "us-east-2", ID: graphql.String(utils.PlaceholderTestUUID().String()), ClientSHA: "old_test_sha", UpdatedAt: time.Now()}}
			expected := subscriptions.ToInstances(tt.expected)

			rows, err := testDBClient.UpdateInstance(context.Background(), mockClient, expected[0])
			if err != nil {
				t.Fatalf("did not expect an error, got %s", err)
			}

			if rows != len(expected) {
				t.Fatalf("incorrect number of rows updated, expected %d rows, inserted %d", len(expected), rows)
			}

			if ok := reflect.DeepEqual(testInstances, tt.expected); !ok {
				t.Fatalf("incorrect instances updated, expected %v, got %v", tt.expected, testInstances)
			}
		})
	}
}
