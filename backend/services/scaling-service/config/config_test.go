// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"reflect"
	"sort"
	"testing"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/internal/sstest"
)

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

			client := sstest.TestClient{TargetFreeMandelboxes: targetFreeMandelboxes}

			if err := Initialize(context.Background(), &client); err != nil {
				t.Fatal("Initialize:", err)
			}

			regions := GetEnabledRegions()

			sort.Strings(regions)
			sort.Strings(test.regions)

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
			client := sstest.TestClient{MandelboxLimit: test.limit}

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
			client := sstest.TestClient{}

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
			"us-west-2":    0, // us-west-2 is not enabled on our test client below
			"ca-central-1": 5,
		}},
	}

	for _, test := range tests {
		t.Run(string(test.env), patchAppEnv(test.env, func(t *testing.T) {
			client := sstest.TestClient{TargetFreeMandelboxes: map[string]int{
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
