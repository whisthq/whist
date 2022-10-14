// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"reflect"
	"testing"

	"github.com/whisthq/whist/backend/services/metadata"
)

// TestGetEnabledRegions ensures that GetEnabledRegions returns the default
// value when we build the scaling service for local development.
func TestGetEnabledRegionsLocal(t *testing.T) {
	patchAppEnv(metadata.EnvLocalDev, func(t *testing.T) {
		if err := Initialize(context.Background(), nil); err != nil {
			t.Fatal("Initialize:", err)
		}

		expected := []string{"us-east-1", "test-region"}
		regions := GetEnabledRegions()

		if !reflect.DeepEqual(regions, expected) {
			t.Errorf("Expected %v, got %v", expected, regions)
		}

		expectedLimit := int32(3)
		mandelboxLimit := GetMandelboxLimitPerUser()

		if !reflect.DeepEqual(mandelboxLimit, expectedLimit) {
			t.Errorf("Expected %v, got %v", expectedLimit, mandelboxLimit)
		}
	})(t)
}
