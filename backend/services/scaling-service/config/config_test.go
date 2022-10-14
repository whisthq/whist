// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"
	"strconv"
	"strings"
	"testing"
	"unsafe"

	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// testClient implements the subscriptions.WhistGraphQLClient interface. We use
// it to mock subscriptions.GraphQLClient.
type testClient struct {
	targetFreeMandelboxes map[string]int
	mandelboxLimit        int32
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
	type entry struct {
		Key   graphql.String `graphql:"key"`
		Value graphql.String `graphql:"value"`
	}

	n := len(t.targetFreeMandelboxes)
	regions := make([]string, 0, n)

	// There are n + 2 entries in the configuration table because one entry is
	// ENABLED_REGIONS, one is MANDELBOX_LIMIT_PER_USER, and there is one entry
	// for each of the n enabled regions that specifies the desired number of
	// free Mandelboxes in that region.
	configs := make([]entry, 0, n+2)

	for region, count := range t.targetFreeMandelboxes {
		regions = append(regions, region)

		suffix := strings.ToUpper(strings.ReplaceAll(region, "-", "_"))
		key := graphql.String(fmt.Sprintf("DESIRED_FREE_MANDELBOXES_%s", suffix))
		value := graphql.String(strconv.FormatInt(int64(count), 10))
		configs = append(configs, entry{key, value})
	}

	regionJson, err := json.Marshal(regions)

	if err != nil {
		return err
	}

	regionJsonString := graphql.String(regionJson)
	limit := graphql.String(strconv.FormatInt(int64(t.mandelboxLimit), 10))

	configs = append(configs, entry{"ENABLED_REGIONS", regionJsonString})
	configs = append(configs, entry{"MANDELBOX_LIMIT_PER_USER", limit})

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

		for _, entry := range configs {
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
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			targetFreeMandelboxes := make(map[string]int)

			for _, region := range test.regions {
				targetFreeMandelboxes[region] = 2
			}

			client := testClient{targetFreeMandelboxes: targetFreeMandelboxes}

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

// TestGetTargetFreeMandelboxes ensures that GetTargetFreeMandelboxes returns
// the correct number of free Mandelboxes for each region.
// GetTargetFreeMandelboxes should return 0 when it is called with the name of
// a region that is not enabled as input.
func TestGetTargetFreeMandelboxes(t *testing.T) {
	tests := []struct {
		env                           metadata.AppEnvironment
		expectedTargetFreeMandelboxes map[string]int
	}{
		{metadata.EnvLocalDev, map[string]int{
			"us-east-1":    2, // initializeLocal only enables us-east-1
			"us-west-1":    0,
			"us-west-2":    0,
			"ca-central-1": 0,
		}},
		{metadata.EnvProd, map[string]int{
			"us-east-1":    3,
			"us-west-1":    4,
			"us-west-2":    0, // us-west-2 is not enabled
			"ca-central-1": 5,
		}},
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			client := testClient{targetFreeMandelboxes: map[string]int{
				"us-east-1":    3,
				"us-west-1":    4,
				"ca-central-1": 5,
			}}

			if err := Initialize(context.TODO(), &client); err != nil {
				t.Fatal("Initialize:", err)
			}

			for region, count := range test.expectedTargetFreeMandelboxes {
				if n := GetTargetFreeMandelboxes(region); n != count {
					t.Errorf("Expected %d, got %d (%s)", count, n, region)
				}
			}
		}))
	}
}
