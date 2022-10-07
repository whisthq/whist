// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
	"unsafe"

	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

// testClient implements the subscriptions.WhistGraphQLClient interface. We use
// it to mock subscriptions.GraphQLClient.
type testClient struct {
	regions        []string
	mandelboxLimit int32
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

	configTable := []struct {
		Key   graphql.String `graphql:"key"`
		Value graphql.String `graphql:"value"`
	}{
		{Key: "ENABLED_REGIONS", Value: graphql.String(regions)},
		{Key: "MANDELBOX_LIMIT_PER_USER", Value: graphql.String(utils.Sprintf("%d", t.mandelboxLimit))},
	}

	var config reflect.Value

	// If reflect.ValueOf(q) is a Pointer, reflect.Indirect() will dereference it,
	// otherwise it will return reflect.ValueOf(q). We do this instead of
	// reflect.TypeOf(q).Elem() just in case reflect.TypeOf(q) is not a Pointer.
	// In that case, Elem() would panic.
	ty := reflect.Indirect(reflect.ValueOf(q)).Type()

	switch ty {
	case reflect.TypeOf(subscriptions.QueryFrontendVersion):
		config = reflect.Indirect(reflect.ValueOf(q)).FieldByName("WhistFrontendVersions")
		entry := subscriptions.WhistFrontendVersion{
			ID:    1,
			Major: 1,
			Minor: 0,
			Micro: 0,
		}
		config.Set(reflect.Append(config, reflect.NewAt(reflect.TypeOf(entry),
			unsafe.Pointer(&entry)).Elem()))

		// Use different cases with fallthrough statements rather than using a
		// single case to avoid having one super long line.
	case reflect.TypeOf(subscriptions.QueryDevConfigurations):
		fallthrough
	case reflect.TypeOf(subscriptions.QueryStagingConfigurations):
		fallthrough
	case reflect.TypeOf(subscriptions.QueryProdConfigurations):

		config = reflect.Indirect(reflect.ValueOf(q)).FieldByName("WhistConfigs")

		for _, entry := range configTable {
			config.Set(reflect.Append(config, reflect.NewAt(reflect.TypeOf(entry),
				unsafe.Pointer(&entry)).Elem()))
		}
	default:
		return fmt.Errorf("Not implemented: %v", ty)
	}

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

// TestGetMandelboxLimit ensures that GetEnabledRegions returns the list of
// enabled regions retrieved from the configuration database.
func TestGetMandelboxLimit(t *testing.T) {
	var tests = []struct {
		env   metadata.AppEnvironment
		limit int32
	}{
		{metadata.EnvDev, 3},
		{metadata.EnvStaging, 3},
		{metadata.EnvProd, 3},
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			client := testClient{mandelboxLimit: test.limit}

			if err := Initialize(context.Background(), &client); err != nil {
				t.Fatal("Initialize:", err)
			}

			limit := GetMandelboxLimitPerUser()

			if !reflect.DeepEqual(limit, test.limit) {
				t.Errorf("Expected %v, got %v", test.limit, limit)
			}
		}))
	}
}

// TestGetFrontendVersion ensures that GetFrontendVersion returns the
// frontend version retrieved from the configuration database.
func TestGetFrontendVersion(t *testing.T) {
	var tests = []struct {
		env     metadata.AppEnvironment
		version string
	}{
		{metadata.EnvDev, "1.0.0-dev-rc.0"},
		{metadata.EnvStaging, "1.0.0-staging-rc.0"},
		{metadata.EnvProd, "1.0.0"},
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			client := testClient{}

			if err := Initialize(context.Background(), &client); err != nil {
				t.Fatal("Initialize:", err)
			}

			version := GetFrontendVersion()

			if !reflect.DeepEqual(version, test.version) {
				t.Errorf("Expected %v, got %v", test.version, version)
			}
		}))
	}
}
