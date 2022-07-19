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

type mockGraphQLClient struct{}

func (mc *mockGraphQLClient) Initialize(bool) error {
	return nil
}

func (mc *mockGraphQLClient) Query(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
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

func (mc *mockGraphQLClient) Mutate(ctx context.Context, query subscriptions.GraphQLQuery, res map[string]interface{}) error {
	return nil
}

func TestGetDevConfigs(t *testing.T) {
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-user-config-key-1": "test-user-config-value-1",
			"test-user-config-key-2": "test-user-config-value-2",
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
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-user-config-key-1": "test-user-config-value-1",
			"test-user-config-key-2": "test-user-config-value-2",
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
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected map[string]string
	}{
		{"Existing configs", "", map[string]string{
			"test-user-config-key-1": "test-user-config-value-1",
			"test-user-config-key-2": "test-user-config-value-2",
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
	mockClient := &mockGraphQLClient{}

	var tests = []struct {
		name     string
		err      string
		expected subscriptions.FrontendVersion
	}{
		{"Existing versions", "", subscriptions.FrontendVersion{}},
		{"Empty versions", "could not find frontend version on config database", subscriptions.FrontendVersion{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			testFrontendVersions = []subscriptions.WhistFrontendVersion{}
			configs, err := GetFrontendVersion(context.Background(), mockClient)
			if err != nil && !strings.Contains(err.Error(), tt.err) {
				t.Errorf("expected error: %s, got: %s", tt.err, err)
			}

			ok := reflect.DeepEqual(configs, tt.expected)
			if !ok {
				t.Fatalf("GetFrontendVersion returned bad result: expected %v, got %v", tt.expected, configs)
			}
		})
	}
}
