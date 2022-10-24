// Copyright (c) 2022 Whist Technologies, Inc.

// Package config provides functions for fetching configuration values from the
// configuration database when the scaling service starts and for reading those
// values while the scaling service is running. Currently, it only reads the
// list of enabled regions from the configuration database, but more
// functionality can be migrated into this module over time.
// config.Initialize() should be called as close as possible to the top of the
// main function.
package config

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// serviceConfig stores service-global configuration values.
type serviceConfig struct {
	// enabledRegions is the list of regions in which users are allowed to request
	// Mandelboxes.
	enabledRegions []string

	// mandelboxLimitPerUser is the maximum number of active mandelboxes a
	// user can have.
	mandelboxLimitPerUser int32

	// frontendVersion represents the current version of the frontend
	// (e.g. "2.6.13").
	frontendVersion string

	// targetFreeMandelboxes maps region names to integers, where the integer
	// associated with each region is the number of Mandelboxes we want to have
	// available at all times. This might differ from the actual number number
	// of Mandelboxes we have available at any given time, but that's what scaling
	// algorithms are for.
	//
	// TODO(owen): It might make more sense to store a mapping from strings to
	// uint32s because we can't have negative Mandelboxes available. I haven't
	// done this yet because it will likely require changing a lot of types and
	// doing a lot of type conversions.
	targetFreeMandelboxes map[string]int
}

// config is a singleton that stores service-global configuration values.
var config serviceConfig

// rw sychronizes access to the configuration singleton.
var rw sync.RWMutex

// Initialize populates the configuration singleton with values from the
// configuration database.
func Initialize(ctx context.Context, client subscriptions.WhistGraphQLClient) error {
	if metadata.IsLocalEnvWithoutDB() {
		return initializeLocal(ctx, client)
	}

	return initialize(ctx, client)
}

// GetEnabledRegions returns a list of regions in which a user may request a
// Mandelbox. Attempting to launch an instance in one of the regions returned
// by this function may still result in an error if the requisite cloud
// infrastructure does not exist in that region.
func GetEnabledRegions() []string {
	rw.RLock()
	defer rw.RUnlock()

	return config.enabledRegions
}

// GetMandelboxLimitPerUser returns the limit of mandelboxes a user can request.
// This includes mandelboxes that are running or in the process of getting ready.
func GetMandelboxLimitPerUser() int32 {
	rw.RLock()
	defer rw.RUnlock()

	return config.mandelboxLimitPerUser
}

// GetFrontendVersion returns the current version number of the frontend as
// reported by the config database.
func GetFrontendVersion() string {
	rw.RLock()
	defer rw.RUnlock()

	return config.frontendVersion
}

// setFrontendVersion sets the frontend version we track locally. It does not update the value in the config database,
// only the configuration variable defined in this file shared between scaling algorithms. This function is only used
// when starting the scaling algorithm, and when the CI has updated the config database.
func SetFrontendVersion(newVersion subscriptions.FrontendVersion) {
	rw.RLock()
	defer rw.RUnlock()

	version := newVersion.String()
	config.frontendVersion = version
}

// GetTargetFreeMandelboxes returns the number of Mandelboxes we want to have
// available all times in a particular region.
func GetTargetFreeMandelboxes(r string) int {
	rw.RLock()
	defer rw.RUnlock()

	if n, ok := config.targetFreeMandelboxes[r]; ok {
		return n
	}

	return 0
}
