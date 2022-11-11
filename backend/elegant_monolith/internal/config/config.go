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
	"time"

	"github.com/hashicorp/go-multierror"
	"github.com/knadh/koanf"
)

// serviceConfig stores service-global configuration values.
type serviceConfig struct {
	// enabledRegions is the list of regions in which users are allowed to request
	// Mandelboxes.
	enabledRegions []string

	// mandelboxLimitPerUser is the maximum number of active mandelboxes a
	// user can have.
	mandelboxLimitPerUser int

	// targetFreeMandelboxes maps region names to integers, where the integer
	// associated with each region is the number of Mandelboxes we want to have
	// available at all times. This might differ from the actual number number
	// of Mandelboxes we have available at any given time, but that's what scaling
	// algorithms are for.
	targetFreeMandelboxes map[string]uint32

	scalingEnabled     bool
	commitHashOverride bool
	useProdLogging     bool

	scalingInterval time.Duration

	databaseConnectionString string
}

// config is a singleton that stores service-global configuration values.
var config serviceConfig

// rw sychronizes access to the configuration singleton.
var rw sync.RWMutex

// Initialize populates the configuration singleton with values from the
// configuration database.
func Initialize(ctx context.Context) error {
	rw.RLock()
	defer rw.RUnlock()

	var k = koanf.New(".")

	var configErr *multierror.Error

	multierror.Append(configErr, getConfigFromFile(k))
	multierror.Append(configErr, getConfigFromFlags(k))
	multierror.Append(configErr, getConfigFromEnv(k))

	return configErr.ErrorOrNil()
}

// GetTargetFreeMandelboxes returns the number of Mandelboxes we want to have
// available all times in a particular region.
func GetTargetFreeMandelboxes(r string) uint32 {
	rw.RLock()
	defer rw.RUnlock()

	if n, ok := config.targetFreeMandelboxes[r]; ok {
		return n
	}

	return 0
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
func GetMandelboxLimitPerUser() int {
	rw.RLock()
	defer rw.RUnlock()

	return config.mandelboxLimitPerUser
}

func GetCommitHashOverride() bool {
	rw.RLock()
	defer rw.RUnlock()

	return config.commitHashOverride
}

func UseProdLogging() bool {
	rw.RLock()
	defer rw.RUnlock()

	return config.useProdLogging
}

func GetScalingEnabled() bool {
	rw.RLock()
	defer rw.RUnlock()

	return config.scalingEnabled
}

func GetScalingInterval() time.Duration {
	rw.RLock()
	defer rw.RUnlock()

	return config.scalingInterval
}

func GetDatabaseURL() string {
	rw.RLock()
	defer rw.RUnlock()

	return config.databaseConnectionString
}
