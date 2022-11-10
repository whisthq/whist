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
	"flag"
	"fmt"
	"log"
	"os"
	"sync"
	"time"

	"github.com/knadh/koanf"
	"github.com/knadh/koanf/parsers/toml"
	"github.com/knadh/koanf/providers/basicflag"
	"github.com/knadh/koanf/providers/file"
)

// serviceConfig stores service-global configuration values.
type serviceConfig struct {
	scalingEnabled bool

	scalingInterval time.Duration

	// enabledRegions is the list of regions in which users are allowed to request
	// Mandelboxes.
	enabledRegions []string

	// mandelboxLimitPerUser is the maximum number of active mandelboxes a
	// user can have.
	mandelboxLimitPerUser int
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

	// Load TOML config
	err := k.Load(file.Provider("config.toml"), toml.Parser())
	if err != nil {
		return fmt.Errorf("error loading config: %s", err)
	}

	// Load command line config
	f, err := setFlags()
	if err != nil {
		return fmt.Errorf("error parsing flags: %s", err)
	}

	err = k.Load(basicflag.Provider(f, "."), nil)
	if err != nil {
		return fmt.Errorf("error loading flags to config: %s", err)
	}
	log.Printf("%v", k.Duration("scalingtime"))

	config.enabledRegions = k.MapKeys("regions")
	config.mandelboxLimitPerUser = k.Int("mandelboxes.limit")
	config.scalingEnabled = k.Bool("scaling")
	config.scalingInterval = k.Duration("scalingtime")

	return nil
}

func setFlags() (*flag.FlagSet, error) {
	var (
		scalingInterval time.Duration
		cleanupPeriod   time.Duration

		scalingEnabled bool
		cleanupEnabled bool
	)

	f := flag.NewFlagSet("config", flag.ContinueOnError)
	f.DurationVar(&scalingInterval, "scalingtime", time.Duration(time.Minute*10), "Interval between when each scaling service runs.")
	f.DurationVar(&cleanupPeriod, "cleantime", time.Duration(time.Minute), "Interval between when each cleaner service runs.")

	f.BoolVar(&scalingEnabled, "scaling", false, "If the application starts the scaling service or not.")
	f.BoolVar(&cleanupEnabled, "cleanup", false, "If the application starts the cleaner service or not.")

	err := f.Parse(os.Args[1:])
	if err != nil {
		return nil, err
	}

	return f, nil
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
