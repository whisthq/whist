// Copyright (c) 2022 Whist Technologies, Inc.

package config

import (
	"context"
	"encoding/json"

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
// it is the only one with access the memory containing the slice. Make sure to
// lock that data before calling this function.
func getEnabledRegions(db map[string]string, regions *[]string) error {
	data, ok := db["ENABLED_REGIONS"]

	if !ok {
		*regions = nil
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

	config = newConfig

	return nil
}

// initializeLocal populates the global configuration singleton with static
// data.
func initializeLocal(_ context.Context, _ subscriptions.WhistGraphQLClient) error {
	config.enabledRegions = []string{"us-east-1"}

	logger.Warningf("Scaling service local build not fetching configuration " +
		"values from the config database. Using static configuration instead.")

	return nil
}
