package dbclient

import (
	"context"
	"reflect"
	"strings"
	"testing"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

var (
	testConfigs          subscriptions.WhistConfigs
	testFrontendVersions []subscriptions.WhistFrontendVersion
)

type mockConfigGraphQLClient struct{}

func (mc *mockConfigGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockConfigGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	switch query := query.(type) {
	case *struct {
		subscriptions.WhistConfigs `graphql:"dev"`
	}:
		query.WhistConfigs = testConfigs
	case *struct {
		subscriptions.WhistConfigs `graphql:"staging"`
	}:
		query.WhistConfigs = testConfigs
	case *struct {
		subscriptions.WhistConfigs `graphql:"prod"`
	}:
		query.WhistConfigs = testConfigs
	case *struct {
		WhistFrontendVersions []subscriptions.WhistFrontendVersion `graphql:"desktop_app_version(where: {id: {_eq: $id}})"`
	}:
		query.WhistFrontendVersions = testFrontendVersions
	default:
	}

	return nil
}

func (mc *mockConfigGraphQLClient) Mutate(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	return nil
}

func TestGetDevConfigs(t *testing.T) {
	mockClient := &mockConfigGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-config-key-1": "test-config-value-1",
			"test-config-key-2": "test-config-value-2",
		}},
		{"Empty configs", "could not find dev configs on database", nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testConfigs = subscriptions.WhistConfigs{}
			for k, v := range tt.expected {
				testConfigs = append(testConfigs, struct {
					Key   graphql.String `graphql:"key"`
					Value graphql.String `graphql:"value"`
				}{
					Key:   graphql.String(k),
					Value: graphql.String(v),
				})
			}

			configs, err := GetDevConfigs(context.Background(), mockClient)
			if err != nil && !strings.Contains(err.Error(), tt.err) {
				t.Errorf("expected error: %s, got: %s", tt.err, err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetDevConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetStagingConfigs(t *testing.T) {
	mockClient := &mockConfigGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-config-key-1": "test-config-value-1",
			"test-config-key-2": "test-config-value-2",
		}},
		{"Empty configs", "could not find staging configs on database", nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testConfigs = subscriptions.WhistConfigs{}
			for k, v := range tt.expected {
				testConfigs = append(testConfigs, struct {
					Key   graphql.String `graphql:"key"`
					Value graphql.String `graphql:"value"`
				}{
					Key:   graphql.String(k),
					Value: graphql.String(v),
				})
			}
			configs, err := GetStagingConfigs(context.Background(), mockClient)
			if err != nil && !strings.Contains(err.Error(), tt.err) {
				t.Errorf("expected error: %s, got: %s", tt.err, err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetStagingConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetProdConfigs(t *testing.T) {
	mockClient := &mockConfigGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-config-key-1": "test-config-value-1",
			"test-config-key-2": "test-config-value-2",
		}},
		{"Empty configs", "could not find prod configs on database", nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testConfigs = subscriptions.WhistConfigs{}
			for k, v := range tt.expected {
				testConfigs = append(testConfigs, struct {
					Key   graphql.String `graphql:"key"`
					Value graphql.String `graphql:"value"`
				}{
					Key:   graphql.String(k),
					Value: graphql.String(v),
				})
			}
			configs, err := GetProdConfigs(context.Background(), mockClient)
			if err != nil && !strings.Contains(err.Error(), tt.err) {
				t.Errorf("expected error: %s, got: %s", tt.err, err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetProdConfigs returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}

func TestGetFrontendVersion(t *testing.T) {
	mockClient := &mockConfigGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected subscriptions.WhistFrontendVersion
	}{
		{"Existing versions", "", subscriptions.WhistFrontendVersion{
			ID:                graphql.Int(1),
			Major:             graphql.Int(1),
			Minor:             graphql.Int(1),
			Micro:             graphql.Int(1),
			DevRC:             graphql.Int(1),
			StagingRC:         graphql.Int(1),
			DevCommitHash:     graphql.String("test_dev_sha"),
			StagingCommitHash: graphql.String("test_staging_sha"),
			ProdCommitHash:    graphql.String("test_prod_sha"),
		}},
		{"Empty versions", "could not find frontend version on config database", subscriptions.WhistFrontendVersion{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testFrontendVersions = []subscriptions.WhistFrontendVersion{tt.expected}

			configs, err := GetFrontendVersion(context.Background(), mockClient)
			if err != nil && !strings.Contains(err.Error(), tt.err) {
				t.Errorf("expected error: %s, got: %s", tt.err, err)
			}

			ok := reflect.DeepEqual(configs, subscriptions.ToFrontendVersion(tt.expected))
			if !ok {
				t.Fatalf("GetFrontendVersion returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}
