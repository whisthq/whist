package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

// These functions are standalone (do not belong to a WhistDBClient implementation) because
// they are only used for populating configuration values before starting the scaling algorithm.

// GetDevConfigs will query the config database's `dev` table and parse the result as a map[string]string.
func GetDevConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryDevConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find dev configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetDevConfigs will query the config database's `staging` table and parse the result as a map[string]string.
func GetStagingConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryStagingConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find staging configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetDevConfigs will query the config database's `prod` table and parse the result as a map[string]string.
func GetProdConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryProdConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find prod configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}
