package dbclient

import (
	"context"
	"reflect"
	"testing"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

var testConfigs []subscriptions.WhistConfigs

type mockGraphQLClient struct{}

func (mc *mockGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	if query != nil {
		query = testConfigs
	} else {
		query = &testConfigs
	}

	return nil
}

func (mc *mockGraphQLClient) Mutate(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	var updateParams []subscriptions.WhistConfigs
	for k, v := range res {
		updateParams = append(updateParams, subscriptions.WhistConfigs{
			struct {
				Key   graphql.String `graphql:"key"`
				Value graphql.String `graphql:"value"`
			}{
				Key:   graphql.String(k),
				Value: graphql.String(v.(string)),
			},
		})
	}

	testConfigs = updateParams
	return nil
}

func TestGetDevConfigs(t *testing.T) {
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      error
		expected []subscriptions.WhistConfigs
	}{
		{"Existing configs", nil, testConfigs},
		{"Empty configs", utils.MakeError(""), []subscriptions.WhistConfigs{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			configs, err := GetDevConfigs(context.Background(), mockClient)
			if err != nil {
				t.Fatalf("failed to get dev configs: %s", err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetDevConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetStagingConfigs(t *testing.T) {
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      error
		expected []subscriptions.WhistConfigs
	}{
		{"Existing configs", nil, testConfigs},
		{"Empty configs", utils.MakeError(""), []subscriptions.WhistConfigs{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			configs, err := GetStagingConfigs(context.Background(), mockClient)
			if err != nil {
				t.Fatalf("failed to get staging configs: %s", err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetStagingConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetProdConfigs(t *testing.T) {
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      error
		expected []subscriptions.WhistConfigs
	}{
		{"Existing configs", nil, testConfigs},
		{"Empty configs", utils.MakeError(""), []subscriptions.WhistConfigs{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			configs, err := GetProdConfigs(context.Background(), mockClient)
			if err != nil {
				t.Fatalf("failed to get prod configs: %s", err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetProdConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetFrontendVersion(t *testing.T) {

}
