// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"encoding/json"
	"reflect"
	"testing"
	"unsafe"

	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// testClient implements the subscriptions.WhistGraphQLClient interface. We use
// it to mock subscriptions.GraphQLClient.
type testClient struct {
	regions []string
}

// Initialize is part of the subscriptions.WhistGraphQLClient interface.
func (*testClient) Initialize(_ bool) error {
	return nil
}

// Mutate is part of the subscriptions.WhistGraphQLClient interface.
func (*testClient) Mutate(_ context.Context, _ subscriptions.GraphQLQuery, _ map[string]interface{}) error {
	return nil
}

// Query is part of the subscriptions.WhistGraphQLClient interface. This
// implementation populates the query struct with mock region data.
func (t *testClient) Query(_ context.Context, q subscriptions.GraphQLQuery, _ map[string]interface{}) error {
	regions, err := json.Marshal(t.regions)

	if err != nil {
		return err
	}

	regionConfig := struct {
		Key   graphql.String `graphql:"key"`
		Value graphql.String `graphql:"value"`
	}{Key: "ENABLED_REGIONS", Value: graphql.String(regions)}

	config := reflect.Indirect(reflect.ValueOf(q)).FieldByName("WhistConfigs")
	config.Set(reflect.Append(config, reflect.NewAt(reflect.TypeOf(regionConfig),
		unsafe.Pointer(&regionConfig)).Elem()))

	return nil
}

// patchAppEnv patches metadata.GetAppEnvironment to simulate running in a
// particular metadata.AppEnvironment for the duration of a single test.
func patchAppEnv(e metadata.AppEnvironment, f func(*testing.T)) func(*testing.T) {
	return func(t *testing.T) {
		var getAppEnv = metadata.GetAppEnvironment

		t.Cleanup(func() {
			metadata.GetAppEnvironment = getAppEnv
		})

		metadata.GetAppEnvironment = func() metadata.AppEnvironment {
			return e
		}

		f(t)
	}
}

// TestGetEnabledRegions ensures that GetEnabledRegions returns the list of
// enabled regions retrieved from the configuration database.
func TestGetEnabledRegions(t *testing.T) {
	var tests = []struct {
		env     metadata.AppEnvironment
		regions []string
	}{
		{metadata.EnvProd, []string{"us-east-1", "us-west-1", "ca-central-1"}},
		{metadata.EnvProd, []string{}},
		{metadata.EnvProd, nil},
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			client := testClient{regions: test.regions}

			if err := Initialize(context.Background(), &client); err != nil {
				t.Fatal("Initialize:", err)
			}

			regions := GetEnabledRegions()

			if !reflect.DeepEqual(regions, test.regions) {
				t.Errorf("Expected %v, got %v", test.regions, regions)
			}
		}))
	}
}
