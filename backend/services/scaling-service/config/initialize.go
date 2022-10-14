// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
	"strings"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// getConfigFromDB fetches service-global configuration values from the
// configuration database.
func getConfigFromDB(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	env := metadata.GetAppEnvironment()

	switch env {
	case metadata.EnvProd:
		return dbclient.GetProdConfigs(ctx, client)
	case metadata.EnvStaging:
		return dbclient.GetStagingConfigs(ctx, client)
	case metadata.EnvDev, metadata.EnvLocalDevWithDB:
		return dbclient.GetDevConfigs(ctx, client)
	default:
		return nil, utils.MakeError("Unexpected app environment '%s'\n", env)
	}
}

// getEnabledRegions extracts the list of regions in which users may request
// Mandelboxes from the data returned by the configuration database and stores
// the result in the string slice pointer provided. This function assumes that
// it is the only one with access to the memory containing the slice. Make sure to
// lock that data before calling this function.
func getEnabledRegions(db map[string]string, regions *[]string) error {
	data, ok := db["ENABLED_REGIONS"]

	if !ok {
		*regions = []string{"us-east-1"}
		logger.Warningf("Configuration key ENABLED_REGIONS not found. Falling "+
			"back to %v", *regions)

		return nil
	}

	var temp []string

	if err := json.Unmarshal([]byte(data), &temp); err != nil {
		return err
	}

	*regions = temp

	logger.Infof("Enabled regions: %v", *regions)

	return nil
}

// getMandelboxLimit extracts the mandelbox limit from the data returned by
// the configuration database and stores the result in the int pointer provided.
// This function assumes that it is the only one with access to the memory containing
// the slice. Make sure to lock that data before calling this function.
func getMandelboxLimit(db map[string]string, mandelboxLimit *int32) error {
	data, ok := db["MANDELBOX_LIMIT_PER_USER"]

	if !ok {
		*mandelboxLimit = 3
		logger.Warningf("Configuration key MANDELBOX_LIMIT_PER_USER not found. Falling "+
			"back to %v", *mandelboxLimit)

		return nil
	}

	var temp int32

	if err := json.Unmarshal([]byte(data), &temp); err != nil {
		return err
	}

	*mandelboxLimit = temp

	logger.Infof("Allowed mandelboxes per user: %v", *mandelboxLimit)

	return nil
}

// getFrontendVersion returns the current version of the frontend, which is initially
// populated from the config database. This value is used by the scaling algorithm to
// determine if the incoming requests come from an outdated frontend, and is part of
// common configuration values shared by the scaling algorithms. Its necessary to grab
// a lock because multiple scaling algorithms read and update it.
func getFrontendVersion(dbVersion subscriptions.FrontendVersion, version *string) {
	if dbVersion == (subscriptions.FrontendVersion{}) {
		*version = "0.0.0"
		logger.Warningf("Got an empty frontend version, falling back to %s", version)
	}

	*version = dbVersion.String()

	logger.Infof("Frontend version: %v", *version)
}

// getFreeMandelboxes
func getFreeMandelboxes(db map[string]string, regions []string, dest *map[string]int) {
	mboxes := make(map[string]int)

	for _, region := range regions {
		var n int
		suffix := strings.ToUpper(strings.ReplaceAll(region, "-", "_"))
		key := fmt.Sprintf("DESIRED_FREE_MANDELBOXES_%s", suffix)
		count, ok := db[key]
		fallback := 2

		if !ok {
			n = fallback
			logger.Errorf("no value specified for configuration key '%s'. Using %d "+
				"by default.", key, n)
		} else if k, err := strconv.Atoi(count); err != nil {
			n = fallback
			logger.Errorf("Failed to parse value for configuration key '%s': %s", key,
				err)
		} else {
			n = k
		}

		mboxes[region] = n
	}

	*dest = mboxes
}

// initialize populates the configuration singleton with values from the
// configuration database.
func initialize(ctx context.Context, client subscriptions.WhistGraphQLClient) error {
	rw.Lock()
	defer rw.Unlock()

	// Copy the existing configuration after acquiring the write lock so we can
	// perform the update atomically.
	newConfig := config

	db, err := getConfigFromDB(ctx, client)

	if err != nil {
		return err
	}

	if err := getEnabledRegions(db, &newConfig.enabledRegions); err != nil {
		return err
	}

	if err := getMandelboxLimit(db, &newConfig.mandelboxLimitPerUser); err != nil {
		return err
	}

	// Get the most recent frontend version from the config database
	dbVersion, err := dbclient.GetFrontendVersion(ctx, client)
	if err != nil {
		logger.Error(err)
	}

	getFrontendVersion(dbVersion, &newConfig.frontendVersion)
	getFreeMandelboxes(db, newConfig.enabledRegions,
		&newConfig.targetFreeMandelboxes)

	config = newConfig

	return nil
}

// initializeLocal populates the global configuration singleton with static
// data.
func initializeLocal(_ context.Context, _ subscriptions.WhistGraphQLClient) error {
	config.enabledRegions = []string{"us-east-1", "test-region"}
	config.mandelboxLimitPerUser = 3
	config.targetFreeMandelboxes = make(map[string]int)

	for _, region := range config.enabledRegions {
		config.targetFreeMandelboxes[region] = 2
	}

	logger.Warningf("Scaling service local build not fetching configuration " +
		"values from the config database. Using static configuration instead.")

	return nil
}
